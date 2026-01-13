#include "editor.h"

#include "brutal/core/logging.h"
#include "brutal/core/platform.h"
#include "brutal/world/player.h"
#include "brutal/renderer/debug_draw.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#ifdef abs
#undef abs
#endif

namespace brutal {

    namespace {

        constexpr i32 kLineHeight = 16;
        constexpr i32 kPanelPadding = 10;
        constexpr i32 kHierarchyWidth = 260;
        constexpr i32 kInspectorWidth = 320;
        constexpr i32 kAssetsHeight = 140;
        constexpr f32 kDragSpeed = 0.01f;

        struct Ray {
            Vec3 origin;
            Vec3 dir;
        };

        bool ray_intersect_aabb(const Ray& ray, const AABB& aabb, f32* out_t) {
            f32 tmin = 0.0f;
            f32 tmax = 1e9f;

            const f32* origin = &ray.origin.x;
            const f32* dir = &ray.dir.x;
            const f32* bmin = &aabb.min.x;
            const f32* bmax = &aabb.max.x;

            for (int i = 0; i < 3; ++i) {
                if (std::abs(dir[i]) < 1e-6f) {
                    if (origin[i] < bmin[i] || origin[i] > bmax[i]) return false;
                }
                else {
                    f32 inv = 1.0f / dir[i];
                    f32 t1 = (bmin[i] - origin[i]) * inv;
                    f32 t2 = (bmax[i] - origin[i]) * inv;
                    if (t1 > t2) std::swap(t1, t2);
                    tmin = std::max(tmin, t1);
                    tmax = std::min(tmax, t2);
                    if (tmin > tmax) return false;
                }
            }

            if (out_t) *out_t = tmin;
            return true;
        }

        Ray make_mouse_ray(const Camera& camera, i32 mouse_x, i32 mouse_y, i32 screen_w, i32 screen_h) {
            f32 ndc_x = (2.0f * (f32)mouse_x / (f32)screen_w) - 1.0f;
            f32 ndc_y = 1.0f - (2.0f * (f32)mouse_y / (f32)screen_h);
            f32 aspect = (f32)screen_w / (f32)screen_h;
            f32 tan_half_fov = tanf(camera.fov * 0.5f);

            Vec3 forward = camera_forward(&camera);
            Vec3 right = camera_right(&camera);
            Vec3 up = vec3_cross(right, forward);

            Vec3 dir = vec3_normalize(forward + right * (ndc_x * aspect * tan_half_fov) + up * (ndc_y * tan_half_fov));
            return { camera.position, dir };
        }

        AABB prop_aabb(const PropEntity& prop) {
            Vec3 size = prop.transform.scale;
            return aabb_from_center_size(prop.transform.position, size);
        }

        bool mouse_in_rect(const InputState* input, i32 x, i32 y, i32 w, i32 h) {
            return input->mouse.x >= x && input->mouse.x <= x + w &&
                input->mouse.y >= y && input->mouse.y <= y + h;
        }

        bool mouse_over_ui(const PlatformState* platform) {
            const InputState* input = &platform->input;
            if (mouse_in_rect(input, kPanelPadding, kPanelPadding, kHierarchyWidth, platform->window_height)) return true;
            if (mouse_in_rect(input, platform->window_width - kInspectorWidth - kPanelPadding, kPanelPadding,
                kInspectorWidth, platform->window_height)) return true;
            if (mouse_in_rect(input, kPanelPadding, platform->window_height - kAssetsHeight,
                kHierarchyWidth, kAssetsHeight + kLineHeight * 2)) return true;
            return false;
        }

        void ui_begin(EditorState* editor) {
            editor->ui_hot_id = 0;
        }

        void ui_end(EditorState* editor, const InputState* input) {
            if (!input->mouse.left.down) {
                editor->ui_active_id = 0;
            }
        }

        bool ui_button(EditorState* editor, const InputState* input, i32 id, i32 x, i32 y, i32 w, const char* label) {
            i32 h = kLineHeight;
            bool hovered = mouse_in_rect(input, x, y, w, h);
            if (hovered) {
                editor->ui_hot_id = id;
                if (input->mouse.left.pressed) {
                    editor->ui_active_id = id;
                    return true;
                }
            }
            Vec3 color = hovered ? Vec3(0.9f, 0.9f, 0.7f) : Vec3(0.8f, 0.8f, 0.8f);
            debug_text_printf(x, y, color, "%s", label);
            return false;
        }

        bool ui_checkbox(EditorState* editor, const InputState* input, i32 id, i32 x, i32 y, const char* label, bool* value) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "[%c] %s", *value ? 'x' : ' ', label);
            if (ui_button(editor, input, id, x, y, 140, buffer)) {
                *value = !*value;
                return true;
            }
            return false;
        }

        bool ui_drag_float(EditorState* editor, const InputState* input, i32 id, i32 x, i32 y, const char* label, f32* value, f32 speed) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s: %.3f", label, *value);
            bool hovered = mouse_in_rect(input, x, y, 200, kLineHeight);
            if (hovered) {
                editor->ui_hot_id = id;
                if (input->mouse.left.pressed) {
                    editor->ui_active_id = id;
                    editor->ui_last_mouse_x = input->mouse.x;
                }
            }
            if (editor->ui_active_id == id && input->mouse.left.down) {
                i32 delta = input->mouse.x - editor->ui_last_mouse_x;
                editor->ui_last_mouse_x = input->mouse.x;
                *value += (f32)delta * speed;
                return true;
            }
            Vec3 color = hovered ? Vec3(0.9f, 0.9f, 0.7f) : Vec3(0.85f, 0.85f, 0.85f);
            debug_text_printf(x, y, color, "%s", buffer);
            return false;
        }

        bool ui_drag_vec3(EditorState* editor, const InputState* input, i32 id, i32 x, i32 y, const char* label, Vec3* value, f32 speed) {
            bool changed = false;
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s", label);
            debug_text_printf(x, y, Vec3(0.8f, 0.9f, 1.0f), "%s", buffer);
            y += kLineHeight;
            changed |= ui_drag_float(editor, input, id + 1, x, y, "X", &value->x, speed);
            y += kLineHeight;
            changed |= ui_drag_float(editor, input, id + 2, x, y, "Y", &value->y, speed);
            y += kLineHeight;
            changed |= ui_drag_float(editor, input, id + 3, x, y, "Z", &value->z, speed);
            return changed;
        }

        void editor_select_none(EditorState* editor) {
            editor->selection_type = EditorState::SelectionType::None;
            editor->selection_index = 0;
        }

        void editor_select(EditorState* editor, EditorState::SelectionType type, u32 index) {
            editor->selection_type = type;
            editor->selection_index = index;
        }

        void editor_apply_snap(EditorState* editor, Vec3* value) {
            if (!editor->snap_enabled) return;
            f32 snap = editor->snap_value;
            value->x = roundf(value->x / snap) * snap;
            value->y = roundf(value->y / snap) * snap;
            value->z = roundf(value->z / snap) * snap;
        }

        bool editor_save_scene(const char* path, const Scene* scene) {
            std::ofstream file(path);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open scene file for save: %s", path);
                return false;
            }

            file << "{\n";
            file << "  \"version\": 1,\n";
            file << "  \"brushes\": [\n";
            for (u32 i = 0; i < scene->brush_count; ++i) {
                const Brush& brush = scene->brushes[i];
                const Vec3 color = brush.faces[0].color;
                file << "    {\"min\": [" << brush.min.x << ", " << brush.min.y << ", " << brush.min.z << "], ";
                file << "\"max\": [" << brush.max.x << ", " << brush.max.y << ", " << brush.max.z << "], ";
                file << "\"flags\": " << brush.flags << ", ";
                file << "\"color\": [" << color.x << ", " << color.y << ", " << color.z << "]}";
                file << (i + 1 < scene->brush_count ? ",\n" : "\n");
            }
            file << "  ],\n";

            file << "  \"props\": [\n";
            for (u32 i = 0; i < scene->prop_count; ++i) {
                const PropEntity& prop = scene->props[i];
                file << "    {\"position\": [" << prop.transform.position.x << ", " << prop.transform.position.y << ", " << prop.transform.position.z << "], ";
                file << "\"rotation\": [" << prop.transform.rotation.x << ", " << prop.transform.rotation.y << ", " << prop.transform.rotation.z << "], ";
                file << "\"scale\": [" << prop.transform.scale.x << ", " << prop.transform.scale.y << ", " << prop.transform.scale.z << "], ";
                file << "\"mesh_id\": " << prop.mesh_id << ", ";
                file << "\"color\": [" << prop.color.x << ", " << prop.color.y << ", " << prop.color.z << "], ";
                file << "\"active\": " << (prop.active ? "true" : "false") << "}";
                file << (i + 1 < scene->prop_count ? ",\n" : "\n");
            }
            file << "  ],\n";

            file << "  \"lights\": {\n";
            file << "    \"ambient\": {\"color\": [" << scene->lights.ambient_color.x << ", " << scene->lights.ambient_color.y << ", " << scene->lights.ambient_color.z << "], ";
            file << "\"intensity\": " << scene->lights.ambient_intensity << "},\n";
            file << "    \"points\": [\n";
            for (u32 i = 0; i < scene->lights.point_light_count; ++i) {
                const PointLight& light = scene->lights.point_lights[i];
                file << "      {\"position\": [" << light.position.x << ", " << light.position.y << ", " << light.position.z << "], ";
                file << "\"color\": [" << light.color.x << ", " << light.color.y << ", " << light.color.z << "], ";
                file << "\"radius\": " << light.radius << ", ";
                file << "\"intensity\": " << light.intensity << ", ";
                file << "\"active\": " << (light.active ? "true" : "false") << "}";
                file << (i + 1 < scene->lights.point_light_count ? ",\n" : "\n");
            }
            file << "    ]\n";
            file << "  }\n";
            file << "}\n";

            LOG_INFO("Scene saved to %s", path);
            return true;
        }

        struct JsonCursor {
            const char* at;
        };

        void json_skip_ws(JsonCursor* c) {
            while (*c->at && (*c->at == ' ' || *c->at == '\n' || *c->at == '\r' || *c->at == '\t')) {
                ++c->at;
            }
        }

        bool json_expect(JsonCursor* c, char ch) {
            json_skip_ws(c);
            if (*c->at != ch) return false;
            ++c->at;
            return true;
        }

        bool json_read_string(JsonCursor* c, std::string* out) {
            json_skip_ws(c);
            if (*c->at != '\"') return false;
            ++c->at;
            std::string value;
            while (*c->at && *c->at != '\"') {
                value.push_back(*c->at);
                ++c->at;
            }
            if (*c->at != '\"') return false;
            ++c->at;
            *out = value;
            return true;
        }

        bool json_read_number(JsonCursor* c, f32* out) {
            json_skip_ws(c);
            char* end = nullptr;
            f64 value = std::strtod(c->at, &end);
            if (end == c->at) return false;
            c->at = end;
            *out = (f32)value;
            return true;
        }

        bool json_read_bool(JsonCursor* c, bool* out) {
            json_skip_ws(c);
            if (strncmp(c->at, "true", 4) == 0) {
                c->at += 4;
                *out = true;
                return true;
            }
            if (strncmp(c->at, "false", 5) == 0) {
                c->at += 5;
                *out = false;
                return true;
            }
            return false;
        }

        bool json_read_vec3(JsonCursor* c, Vec3* out) {
            if (!json_expect(c, '[')) return false;
            if (!json_read_number(c, &out->x)) return false;
            if (!json_expect(c, ',')) return false;
            if (!json_read_number(c, &out->y)) return false;
            if (!json_expect(c, ',')) return false;
            if (!json_read_number(c, &out->z)) return false;
            if (!json_expect(c, ']')) return false;
            return true;
        }

        bool json_skip_value(JsonCursor* c) {
            json_skip_ws(c);
            if (*c->at == '{') {
                int depth = 0;
                do {
                    if (*c->at == '{') depth++;
                    if (*c->at == '}') depth--;
                    ++c->at;
                } while (*c->at && depth > 0);
                return true;
            }
            if (*c->at == '[') {
                int depth = 0;
                do {
                    if (*c->at == '[') depth++;
                    if (*c->at == ']') depth--;
                    ++c->at;
                } while (*c->at && depth > 0);
                return true;
            }
            if (*c->at == '\"') {
                std::string temp;
                return json_read_string(c, &temp);
            }
            bool b = false;
            if (json_read_bool(c, &b)) return true;
            f32 n = 0;
            return json_read_number(c, &n);
        }

        bool json_read_brush(JsonCursor* c, Brush* brush) {
            if (!json_expect(c, '{')) return false;
            while (true) {
                std::string key;
                if (!json_read_string(c, &key)) return false;
                if (!json_expect(c, ':')) return false;
                if (key == "min") {
                    if (!json_read_vec3(c, &brush->min)) return false;
                }
                else if (key == "max") {
                    if (!json_read_vec3(c, &brush->max)) return false;
                }
                else if (key == "flags") {
                    f32 flags = 0.0f;
                    if (!json_read_number(c, &flags)) return false;
                    brush->flags = (u32)flags;
                }
                else if (key == "color") {
                    Vec3 color;
                    if (!json_read_vec3(c, &color)) return false;
                    for (int i = 0; i < 6; ++i) brush->faces[i].color = color;
                }
                else {
                    if (!json_skip_value(c)) return false;
                }
                json_skip_ws(c);
                if (*c->at == ',') {
                    ++c->at;
                    continue;
                }
                if (*c->at == '}') {
                    ++c->at;
                    break;
                }
            }
            return true;
        }

        bool json_read_prop(JsonCursor* c, PropEntity* prop) {
            if (!json_expect(c, '{')) return false;
            prop->active = true;
            while (true) {
                std::string key;
                if (!json_read_string(c, &key)) return false;
                if (!json_expect(c, ':')) return false;
                if (key == "position") {
                    if (!json_read_vec3(c, &prop->transform.position)) return false;
                }
                else if (key == "rotation") {
                    if (!json_read_vec3(c, &prop->transform.rotation)) return false;
                }
                else if (key == "scale") {
                    if (!json_read_vec3(c, &prop->transform.scale)) return false;
                }
                else if (key == "mesh_id") {
                    f32 mesh = 0.0f;
                    if (!json_read_number(c, &mesh)) return false;
                    prop->mesh_id = (u32)mesh;
                }
                else if (key == "color") {
                    if (!json_read_vec3(c, &prop->color)) return false;
                }
                else if (key == "active") {
                    if (!json_read_bool(c, &prop->active)) return false;
                }
                else {
                    if (!json_skip_value(c)) return false;
                }
                json_skip_ws(c);
                if (*c->at == ',') {
                    ++c->at;
                    continue;
                }
                if (*c->at == '}') {
                    ++c->at;
                    break;
                }
            }
            return true;
        }

        bool json_read_point_light(JsonCursor* c, PointLight* light) {
            if (!json_expect(c, '{')) return false;
            light->active = true;
            while (true) {
                std::string key;
                if (!json_read_string(c, &key)) return false;
                if (!json_expect(c, ':')) return false;
                if (key == "position") {
                    if (!json_read_vec3(c, &light->position)) return false;
                }
                else if (key == "color") {
                    if (!json_read_vec3(c, &light->color)) return false;
                }
                else if (key == "radius") {
                    if (!json_read_number(c, &light->radius)) return false;
                }
                else if (key == "intensity") {
                    if (!json_read_number(c, &light->intensity)) return false;
                }
                else if (key == "active") {
                    if (!json_read_bool(c, &light->active)) return false;
                }
                else {
                    if (!json_skip_value(c)) return false;
                }
                json_skip_ws(c);
                if (*c->at == ',') {
                    ++c->at;
                    continue;
                }
                if (*c->at == '}') {
                    ++c->at;
                    break;
                }
            }
            return true;
        }

        bool editor_load_scene(const char* path, Scene* scene) {
            std::ifstream file(path);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open scene file for load: %s", path);
                return false;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            JsonCursor cursor{ content.c_str() };
            if (!json_expect(&cursor, '{')) return false;

            scene_clear(scene);
            light_environment_init(&scene->lights);

            while (true) {
                std::string key;
                if (!json_read_string(&cursor, &key)) return false;
                if (!json_expect(&cursor, ':')) return false;
                if (key == "brushes") {
                    if (!json_expect(&cursor, '[')) return false;
                    json_skip_ws(&cursor);
                    while (*cursor.at && *cursor.at != ']') {
                        if (scene->brush_count >= scene->brush_capacity) return false;
                        Brush* brush = &scene->brushes[scene->brush_count++];
                        if (!json_read_brush(&cursor, brush)) return false;
                        json_skip_ws(&cursor);
                        if (*cursor.at == ',') {
                            ++cursor.at;
                            json_skip_ws(&cursor);
                        }
                    }
                    if (!json_expect(&cursor, ']')) return false;
                }
                else if (key == "props") {
                    if (!json_expect(&cursor, '[')) return false;
                    json_skip_ws(&cursor);
                    while (*cursor.at && *cursor.at != ']') {
                        if (scene->prop_count >= scene->prop_capacity) return false;
                        PropEntity* prop = &scene->props[scene->prop_count++];
                        prop->transform = transform_default();
                        if (!json_read_prop(&cursor, prop)) return false;
                        json_skip_ws(&cursor);
                        if (*cursor.at == ',') {
                            ++cursor.at;
                            json_skip_ws(&cursor);
                        }
                    }
                    if (!json_expect(&cursor, ']')) return false;
                }
                else if (key == "lights") {
                    if (!json_expect(&cursor, '{')) return false;
                    while (true) {
                        std::string light_key;
                        if (!json_read_string(&cursor, &light_key)) return false;
                        if (!json_expect(&cursor, ':')) return false;
                        if (light_key == "ambient") {
                            if (!json_expect(&cursor, '{')) return false;
                            while (true) {
                                std::string ambient_key;
                                if (!json_read_string(&cursor, &ambient_key)) return false;
                                if (!json_expect(&cursor, ':')) return false;
                                if (ambient_key == "color") {
                                    if (!json_read_vec3(&cursor, &scene->lights.ambient_color)) return false;
                                }
                                else if (ambient_key == "intensity") {
                                    if (!json_read_number(&cursor, &scene->lights.ambient_intensity)) return false;
                                }
                                else {
                                    if (!json_skip_value(&cursor)) return false;
                                }
                                json_skip_ws(&cursor);
                                if (*cursor.at == ',') {
                                    ++cursor.at;
                                    continue;
                                }
                                if (*cursor.at == '}') {
                                    ++cursor.at;
                                    break;
                                }
                            }
                        }
                        else if (light_key == "points") {
                            if (!json_expect(&cursor, '[')) return false;
                            json_skip_ws(&cursor);
                            scene->lights.point_light_count = 0;
                            while (*cursor.at && *cursor.at != ']') {
                                if (scene->lights.point_light_count >= MAX_POINT_LIGHTS) return false;
                                PointLight* light = &scene->lights.point_lights[scene->lights.point_light_count++];
                                if (!json_read_point_light(&cursor, light)) return false;
                                json_skip_ws(&cursor);
                                if (*cursor.at == ',') {
                                    ++cursor.at;
                                    json_skip_ws(&cursor);
                                }
                            }
                            if (!json_expect(&cursor, ']')) return false;
                        }
                        else {
                            if (!json_skip_value(&cursor)) return false;
                        }
                        json_skip_ws(&cursor);
                        if (*cursor.at == ',') {
                            ++cursor.at;
                            continue;
                        }
                        if (*cursor.at == '}') {
                            ++cursor.at;
                            break;
                        }
                    }
                }
                else {
                    if (!json_skip_value(&cursor)) return false;
                }
                json_skip_ws(&cursor);
                if (*cursor.at == ',') {
                    ++cursor.at;
                    continue;
                }
                if (*cursor.at == '}') {
                    ++cursor.at;
                    break;
                }
            }

            scene->world_mesh_dirty = true;
            LOG_INFO("Scene loaded from %s", path);
            return true;
        }

        void editor_update_selection(EditorState* editor, Scene* scene, PlatformState* platform) {
            const InputState* input = &platform->input;
            if (!input->mouse.left.pressed) return;

            Ray ray = make_mouse_ray(editor->camera, input->mouse.x, input->mouse.y,
                platform->window_width, platform->window_height);

            f32 best_t = 1e9f;
            EditorState::SelectionType best_type = EditorState::SelectionType::None;
            u32 best_index = 0;

            for (u32 i = 0; i < scene->brush_count; ++i) {
                f32 t = 0.0f;
                if (ray_intersect_aabb(ray, brush_to_aabb(&scene->brushes[i]), &t) && t < best_t) {
                    best_t = t;
                    best_type = EditorState::SelectionType::Brush;
                    best_index = i;
                }
            }

            for (u32 i = 0; i < scene->prop_count; ++i) {
                f32 t = 0.0f;
                if (ray_intersect_aabb(ray, prop_aabb(scene->props[i]), &t) && t < best_t) {
                    best_t = t;
                    best_type = EditorState::SelectionType::Prop;
                    best_index = i;
                }
            }

            if (best_type == EditorState::SelectionType::None) {
                editor_select_none(editor);
            }
            else {
                editor_select(editor, best_type, best_index);
            }
        }

        void editor_apply_gizmo(EditorState* editor, Scene* scene, PlatformState* platform) {
            if (editor->selection_type == EditorState::SelectionType::None) return;
            if (!platform->mouse_captured) return;

            const InputState* input = &platform->input;
            f32 dx = (f32)input->mouse.delta_x;
            f32 dy = (f32)input->mouse.delta_y;
            if (dx == 0.0f && dy == 0.0f) return;

            Vec3 right = camera_right(&editor->camera);
            Vec3 up = vec3_cross(right, camera_forward(&editor->camera));

            if (editor->selection_type == EditorState::SelectionType::Prop) {
                PropEntity& prop = scene->props[editor->selection_index];
                if (editor->gizmo_mode == EditorState::GizmoMode::Translate) {
                    prop.transform.position = prop.transform.position + right * (dx * 0.01f) - up * (dy * 0.01f);
                    editor_apply_snap(editor, &prop.transform.position);
                }
                else if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) {
                    prop.transform.rotation.y += dx * 0.005f;
                    prop.transform.rotation.x += dy * 0.005f;
                }
                else if (editor->gizmo_mode == EditorState::GizmoMode::Scale) {
                    f32 scale_delta = (dx - dy) * 0.005f;
                    prop.transform.scale = prop.transform.scale + Vec3(scale_delta, scale_delta, scale_delta);
                    prop.transform.scale.x = std::max(prop.transform.scale.x, 0.05f);
                    prop.transform.scale.y = std::max(prop.transform.scale.y, 0.05f);
                    prop.transform.scale.z = std::max(prop.transform.scale.z, 0.05f);
                }
            }
            else if (editor->selection_type == EditorState::SelectionType::Brush) {
                Brush& brush = scene->brushes[editor->selection_index];
                Vec3 center = aabb_center(brush_to_aabb(&brush));
                Vec3 size = aabb_half_size(brush_to_aabb(&brush)) * 2.0f;
                if (editor->gizmo_mode == EditorState::GizmoMode::Translate) {
                    center = center + right * (dx * 0.01f) - up * (dy * 0.01f);
                    editor_apply_snap(editor, &center);
                }
                else if (editor->gizmo_mode == EditorState::GizmoMode::Scale) {
                    f32 scale_delta = (dx - dy) * 0.01f;
                    size = size + Vec3(scale_delta, scale_delta, scale_delta);
                    size.x = std::max(size.x, 0.1f);
                    size.y = std::max(size.y, 0.1f);
                    size.z = std::max(size.z, 0.1f);
                }
                Vec3 half = size * 0.5f;
                brush.min = center - half;
                brush.max = center + half;
                editor->rebuild_world = true;
                editor->rebuild_collision = true;
            }
            else if (editor->selection_type == EditorState::SelectionType::Light) {
                PointLight& light = scene->lights.point_lights[editor->selection_index];
                light.position = light.position + right * (dx * 0.01f) - up * (dy * 0.01f);
                editor_apply_snap(editor, &light.position);
            }
        }

        void editor_draw_hierarchy(EditorState* editor, Scene* scene, PlatformState* platform) {
            const InputState* input = &platform->input;
            i32 x = kPanelPadding;
            i32 y = kPanelPadding;
            debug_text_printf(x, y, Vec3(0.9f, 0.8f, 0.6f), "Hierarchy");
            y += kLineHeight;

            for (u32 i = 0; i < scene->brush_count; ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Brush %u", i);
                bool clicked = ui_button(editor, input, 1000 + (i32)i, x, y, kHierarchyWidth, label);
                if (clicked) {
                    editor_select(editor, EditorState::SelectionType::Brush, i);
                }
                y += kLineHeight;
            }

            for (u32 i = 0; i < scene->prop_count; ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Prop %u", i);
                bool clicked = ui_button(editor, input, 2000 + (i32)i, x, y, kHierarchyWidth, label);
                if (clicked) {
                    editor_select(editor, EditorState::SelectionType::Prop, i);
                }
                y += kLineHeight;
            }

            for (u32 i = 0; i < scene->lights.point_light_count; ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Light %u", i);
                bool clicked = ui_button(editor, input, 3000 + (i32)i, x, y, kHierarchyWidth, label);
                if (clicked) {
                    editor_select(editor, EditorState::SelectionType::Light, i);
                }
                y += kLineHeight;
            }
        }

        void editor_draw_inspector(EditorState* editor, Scene* scene, PlatformState* platform) {
            const InputState* input = &platform->input;
            i32 x = platform->window_width - kInspectorWidth - kPanelPadding;
            i32 y = kPanelPadding;
            debug_text_printf(x, y, Vec3(0.9f, 0.8f, 0.6f), "Inspector");
            y += kLineHeight;

            if (editor->selection_type == EditorState::SelectionType::None) {
                debug_text_printf(x, y, Vec3(0.7f, 0.7f, 0.7f), "No selection");
                return;
            }

            if (editor->selection_type == EditorState::SelectionType::Brush) {
                Brush& brush = scene->brushes[editor->selection_index];
                Vec3 center = aabb_center(brush_to_aabb(&brush));
                Vec3 size = aabb_half_size(brush_to_aabb(&brush)) * 2.0f;
                bool changed = ui_drag_vec3(editor, input, 4000, x, y, "Center", &center, kDragSpeed);
                y += kLineHeight * 4;
                changed |= ui_drag_vec3(editor, input, 4010, x, y, "Size", &size, kDragSpeed);
                y += kLineHeight * 4;
                if (changed) {
                    size.x = std::max(size.x, 0.1f);
                    size.y = std::max(size.y, 0.1f);
                    size.z = std::max(size.z, 0.1f);
                    Vec3 half = size * 0.5f;
                    brush.min = center - half;
                    brush.max = center + half;
                    editor->rebuild_world = true;
                    editor->rebuild_collision = true;
                }
                bool invisible = (brush.flags & BRUSH_INVISIBLE) != 0;
                if (ui_checkbox(editor, input, 4020, x, y, "Invisible", &invisible)) {
                    if (invisible) brush.flags |= BRUSH_INVISIBLE; else brush.flags &= ~BRUSH_INVISIBLE;
                    editor->rebuild_world = true;
                }
                y += kLineHeight;
            }
            else if (editor->selection_type == EditorState::SelectionType::Prop) {
                PropEntity& prop = scene->props[editor->selection_index];
                bool changed = ui_drag_vec3(editor, input, 4100, x, y, "Position", &prop.transform.position, kDragSpeed);
                y += kLineHeight * 4;
                changed |= ui_drag_vec3(editor, input, 4110, x, y, "Rotation", &prop.transform.rotation, kDragSpeed * 0.5f);
                y += kLineHeight * 4;
                changed |= ui_drag_vec3(editor, input, 4120, x, y, "Scale", &prop.transform.scale, kDragSpeed);
                y += kLineHeight * 4;
                bool active = prop.active;
                if (ui_checkbox(editor, input, 4130, x, y, "Active", &active)) {
                    prop.active = active;
                }
                y += kLineHeight;
                if (changed) {
                    editor_apply_snap(editor, &prop.transform.position);
                    prop.transform.scale.x = std::max(prop.transform.scale.x, 0.05f);
                    prop.transform.scale.y = std::max(prop.transform.scale.y, 0.05f);
                    prop.transform.scale.z = std::max(prop.transform.scale.z, 0.05f);
                }
            }
            else if (editor->selection_type == EditorState::SelectionType::Light) {
                PointLight& light = scene->lights.point_lights[editor->selection_index];
                bool changed = ui_drag_vec3(editor, input, 4200, x, y, "Position", &light.position, kDragSpeed);
                y += kLineHeight * 4;
                changed |= ui_drag_vec3(editor, input, 4210, x, y, "Color", &light.color, 0.005f);
                y += kLineHeight * 4;
                changed |= ui_drag_float(editor, input, 4220, x, y, "Radius", &light.radius, 0.01f);
                y += kLineHeight;
                changed |= ui_drag_float(editor, input, 4230, x, y, "Intensity", &light.intensity, 0.01f);
                y += kLineHeight;
                bool active = light.active;
                if (ui_checkbox(editor, input, 4240, x, y, "Active", &active)) {
                    light.active = active;
                }
                y += kLineHeight;
                if (changed) {
                    editor_apply_snap(editor, &light.position);
                }
            }

            const char* mode = (editor->gizmo_mode == EditorState::GizmoMode::Translate)
                ? "Translate (W)" : (editor->gizmo_mode == EditorState::GizmoMode::Rotate) ? "Rotate (E)" : "Scale (R)";
            debug_text_printf(x, y, Vec3(0.7f, 0.9f, 0.7f), "Mode: %s", mode);
            y += kLineHeight;

            bool snap = editor->snap_enabled;
            if (ui_checkbox(editor, input, 4300, x, y, "Snap", &snap)) {
                editor->snap_enabled = snap;
            }
            y += kLineHeight;
            ui_drag_float(editor, input, 4310, x, y, "Snap Value", &editor->snap_value, 0.01f);
        }

        void editor_draw_assets(EditorState* editor, Scene* scene, PlatformState* platform) {
            const InputState* input = &platform->input;
            i32 x = kPanelPadding;
            i32 y = platform->window_height - kAssetsHeight;
            debug_text_printf(x, y, Vec3(0.9f, 0.8f, 0.6f), "Assets / Spawn");
            y += kLineHeight;

            if (ui_button(editor, input, 5000, x, y, 200, "Add Prop Cube")) {
                scene_add_prop(scene, editor->camera.position + camera_forward(&editor->camera) * 2.0f,
                    Vec3(0.5f, 0.5f, 0.5f), MESH_CUBE, Vec3(0.6f, 0.6f, 0.6f));
            }
            y += kLineHeight;

            if (ui_button(editor, input, 5001, x, y, 200, "Add Pillar Brush")) {
                Vec3 center = editor->camera.position + camera_forward(&editor->camera) * 4.0f;
                Vec3 size(0.4f, 3.0f, 0.4f);
                scene_add_brush(scene, center - size * 0.5f, center + size * 0.5f, BRUSH_SOLID, Vec3(0.15f, 0.12f, 0.1f));
                editor->rebuild_world = true;
                editor->rebuild_collision = true;
            }
            y += kLineHeight;

            if (ui_button(editor, input, 5002, x, y, 200, "Add Wall Brush")) {
                Vec3 center = editor->camera.position + camera_forward(&editor->camera) * 5.0f;
                Vec3 size(3.0f, 2.5f, 0.3f);
                scene_add_brush(scene, center - size * 0.5f, center + size * 0.5f, BRUSH_SOLID, Vec3(0.12f, 0.1f, 0.08f));
                editor->rebuild_world = true;
                editor->rebuild_collision = true;
            }
            y += kLineHeight;

            if (ui_button(editor, input, 5003, x, y, 200, "Add Point Light")) {
                light_environment_add_point(&scene->lights,
                    editor->camera.position + camera_forward(&editor->camera) * 2.0f,
                    Vec3(1.0f, 0.7f, 0.5f), 4.0f, 1.5f);
            }
        }

        void editor_draw_top_bar(EditorState* editor, Scene* scene, PlatformState* platform) {
            const InputState* input = &platform->input;
            i32 x = kPanelPadding;
            i32 y = platform->window_height - kAssetsHeight - kLineHeight * 2;
            if (ui_button(editor, input, 6000, x, y, 120, "Save (Ctrl+S)")) {
                editor_save_scene(editor->scene_path, scene);
            }
            x += 140;
            if (ui_button(editor, input, 6001, x, y, 120, "Load (Ctrl+O)")) {
                editor_load_scene(editor->scene_path, scene);
                editor->rebuild_world = true;
                editor->rebuild_collision = true;
                editor_select_none(editor);
            }
        }

    }

    void editor_init(EditorState* editor) {
        editor->active = false;
        camera_init(&editor->camera);
        editor->camera.position = Vec3(0, 2.0f, 6.0f);
        editor->move_speed = 6.0f;
        editor->look_sensitivity = 0.0025f;
        editor->selection_type = EditorState::SelectionType::None;
        editor->selection_index = 0;
        editor->gizmo_mode = EditorState::GizmoMode::Translate;
        editor->snap_enabled = false;
        editor->snap_value = 0.5f;
        editor->show_grid = true;
        std::snprintf(editor->scene_path, sizeof(editor->scene_path), "scene.json");
        editor->rebuild_world = false;
        editor->rebuild_collision = false;
        editor->ui_active_id = 0;
        editor->ui_hot_id = 0;
        editor->ui_last_mouse_x = 0;
    }

    void editor_set_active(EditorState* editor, bool active, PlatformState* platform, Player* player) {
        editor->active = active;
        if (active) {
            editor->camera = player->camera;
            platform_set_mouse_capture(platform, false);
        }
        else {
            player->camera = editor->camera;
        }
    }

    void editor_update(EditorState* editor, Scene* scene, PlatformState* platform, f32 dt) {
        const InputState* input = &platform->input;

        if (platform_key_pressed(input, KEY_W)) editor->gizmo_mode = EditorState::GizmoMode::Translate;
        if (platform_key_pressed(input, KEY_E)) editor->gizmo_mode = EditorState::GizmoMode::Rotate;
        if (platform_key_pressed(input, KEY_R)) editor->gizmo_mode = EditorState::GizmoMode::Scale;

        bool ctrl_down = platform_key_down(input, KEY_CONTROL) || platform_key_down(input, KEY_LCONTROL);
        if (ctrl_down && platform_key_pressed(input, KEY_S)) {
            editor_save_scene(editor->scene_path, scene);
        }
        if (ctrl_down && platform_key_pressed(input, KEY_O)) {
            if (editor_load_scene(editor->scene_path, scene)) {
                editor->rebuild_world = true;
                editor->rebuild_collision = true;
                editor_select_none(editor);
            }
        }

        if (input->mouse.right.pressed && !mouse_over_ui(platform)) {
            platform_set_mouse_capture(platform, true);
        }
        if (input->mouse.right.released) {
            platform_set_mouse_capture(platform, false);
        }

        if (platform->mouse_captured) {
            f32 dyaw = (f32)input->mouse.delta_x * editor->look_sensitivity;
            f32 dpitch = (f32)(-input->mouse.delta_y) * editor->look_sensitivity;
            camera_rotate(&editor->camera, dyaw, dpitch);
        }

        Vec3 forward = camera_forward(&editor->camera);
        Vec3 right = camera_right(&editor->camera);
        Vec3 movement(0, 0, 0);
        if (platform_key_down(input, KEY_W)) movement = movement + forward;
        if (platform_key_down(input, KEY_S)) movement = movement - forward;
        if (platform_key_down(input, KEY_D)) movement = movement + right;
        if (platform_key_down(input, KEY_A)) movement = movement - right;
        if (platform_key_down(input, KEY_SPACE)) movement.y += 1.0f;
        if (platform_key_down(input, KEY_Q)) movement.y -= 1.0f;

        if (vec3_length(movement) > 0.01f) {
            movement = vec3_normalize(movement);
            editor->camera.position = editor->camera.position + movement * (editor->move_speed * dt);
        }

        if (!platform->mouse_captured && input->mouse.left.pressed && !mouse_over_ui(platform)) {
            editor_update_selection(editor, scene, platform);
        }

        editor_apply_gizmo(editor, scene, platform);
    }

    void editor_draw_ui(EditorState* editor, Scene* scene, PlatformState* platform) {
        ui_begin(editor);
        editor_draw_hierarchy(editor, scene, platform);
        editor_draw_inspector(editor, scene, platform);
        editor_draw_assets(editor, scene, platform);
        editor_draw_top_bar(editor, scene, platform);
        ui_end(editor, &platform->input);

        if (editor->selection_type == EditorState::SelectionType::Brush) {
            const Brush& brush = scene->brushes[editor->selection_index];
            debug_wire_box(aabb_center(brush_to_aabb(&brush)), aabb_half_size(brush_to_aabb(&brush)) * 2.0f, Vec3(1, 0.8f, 0.2f));
        }
        else if (editor->selection_type == EditorState::SelectionType::Prop) {
            const PropEntity& prop = scene->props[editor->selection_index];
            debug_wire_box(prop.transform.position, prop.transform.scale, Vec3(0.3f, 1.0f, 0.5f));
        }
        else if (editor->selection_type == EditorState::SelectionType::Light) {
            const PointLight& light = scene->lights.point_lights[editor->selection_index];
            debug_wire_box(light.position, Vec3(0.2f, 0.2f, 0.2f), Vec3(1.0f, 0.9f, 0.4f));
        }
    }

    bool editor_scene_needs_rebuild(const EditorState* editor) {
        return editor->rebuild_world || editor->rebuild_collision;
    }

    void editor_clear_rebuild_flag(EditorState* editor) {
        editor->rebuild_world = false;
        editor->rebuild_collision = false;
    }

}