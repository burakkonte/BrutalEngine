#include "editor.h"

#include "brutal/core/logging.h"
#include "brutal/core/platform.h"
#include "brutal/world/player.h"
#include "brutal/renderer/debug_draw.h"
#include "brutal/math/quat.h"

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

        constexpr i32 kInvalidEntityId = -1;
        constexpr f32 kGizmoScreenFactor = 0.15f;
        constexpr f32 kGizmoMinScale = 0.25f;
        constexpr f32 kGizmoHitFactor = 0.15f;
        constexpr f32 kRadiansToDegrees = 57.295779513f;
        constexpr f32 kDegreesToRadians = 0.0174532925f;
        constexpr f32 kTransformEpsilon = 1e-4f;

        constexpr i32 kInspectorPositionId = 7000;
        constexpr i32 kInspectorRotationId = 7020;
        constexpr i32 kInspectorScaleId = 7040;
        constexpr i32 kInspectorResetPositionId = 7060;
        constexpr i32 kInspectorResetRotationId = 7070;
        constexpr i32 kInspectorResetScaleId = 7080;
        constexpr i32 kInspectorSnapTranslateId = 7090;
        constexpr i32 kInspectorSnapTranslateValueId = 7091;
        constexpr i32 kInspectorSnapRotateId = 7100;
        constexpr i32 kInspectorSnapRotateValueId = 7101;
        constexpr i32 kInspectorSnapScaleId = 7110;
        constexpr i32 kInspectorSnapScaleValueId = 7111;

        struct Ray {
            Vec3 origin;
            Vec3 dir;
        };

        struct Plane {
            Vec3 normal;
            f32 d;
        };

        Ray RayFromPerspectiveNDC(const Camera& camera, f32 ndc_x, f32 ndc_y, f32 aspect) {
            f32 tan_half_fov = tanf(camera.fov * 0.5f);
            Vec3 forward = camera_forward(&camera);
            Vec3 right = camera_right(&camera);
            Vec3 up = vec3_cross(right, forward);
            Vec3 dir = vec3_normalize(forward + right * (ndc_x * aspect * tan_half_fov) + up * (ndc_y * tan_half_fov));
            return { camera.position, dir };
        }

        Ray RayFromOrthoNDC(const Camera& camera, f32 ndc_x, f32 ndc_y, f32 aspect, f32 ortho_size) {
            Vec3 forward = camera_forward(&camera);
            Vec3 right = camera_right(&camera);
            Vec3 up = vec3_cross(right, forward);
            f32 half_height = ortho_size;
            f32 half_width = ortho_size * aspect;
            Vec3 origin = camera.position + right * (ndc_x * half_width) + up * (ndc_y * half_height) + forward * camera.near_plane;
            return { origin, forward };
        }

        bool ray_segment_distance(const Ray& ray, const Vec3& a, const Vec3& b, f32* out_distance, f32* out_ray_t, f32* out_seg_t);
        i32 pack_entity_id(SelectionType type, u32 index) {
            return ((i32)type << 24) | (i32)index;
        }

        f32 clamp_f32(f32 value, f32 min_value, f32 max_value) {
            return std::max(min_value, std::min(value, max_value));
        }

        f32 radians_to_degrees(f32 radians) {
            return radians * kRadiansToDegrees;
        }

        f32 degrees_to_radians(f32 degrees) {
            return degrees * kDegreesToRadians;
        }

        Vec3 radians_to_degrees(const Vec3& radians) {
            return Vec3(radians_to_degrees(radians.x), radians_to_degrees(radians.y), radians_to_degrees(radians.z));
        }

        Vec3 degrees_to_radians(const Vec3& degrees) {
            return Vec3(degrees_to_radians(degrees.x), degrees_to_radians(degrees.y), degrees_to_radians(degrees.z));
        }

        bool vec3_near_equal(const Vec3& a, const Vec3& b, f32 epsilon) {
            return fabsf(a.x - b.x) <= epsilon && fabsf(a.y - b.y) <= epsilon && fabsf(a.z - b.z) <= epsilon;
        }

        bool quat_near_equal(const Quat& a, const Quat& b, f32 epsilon) {
            f32 dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            return fabsf(dot) >= (1.0f - epsilon);
        }

        bool transform_near_equal(const Transform& a, const Transform& b, f32 epsilon) {
            return vec3_near_equal(a.position, b.position, epsilon) &&
                vec3_near_equal(a.scale, b.scale, epsilon) &&
                quat_near_equal(a.rotation, b.rotation, epsilon);
        }

        Plane plane_from_point_normal(const Vec3& point, const Vec3& normal) {
            Vec3 n = vec3_normalize(normal);
            return { n, -vec3_dot(n, point) };
        }

        bool ray_plane_intersect(const Ray& ray, const Plane& plane, f32* out_t, Vec3* out_point) {
            f32 denom = vec3_dot(plane.normal, ray.dir);
            if (fabsf(denom) < 1e-6f) return false;
            f32 t = -(vec3_dot(plane.normal, ray.origin) + plane.d) / denom;
            if (t < 0.0f) return false;
            if (out_t) *out_t = t;
            if (out_point) *out_point = ray.origin + ray.dir * t;
            return true;
        }

        bool RayPlaneIntersect(const Ray& ray, const Plane& plane, f32* out_t, Vec3* out_point) {
            return ray_plane_intersect(ray, plane, out_t, out_point);
        }

        bool RayRingHitTest(const Ray& ray, const Vec3& center, const Vec3& axis, f32 radius, f32 thickness, Vec3* out_hit) {
            Plane plane = plane_from_point_normal(center, axis);
            Vec3 hit;
            if (!ray_plane_intersect(ray, plane, nullptr, &hit)) return false;
            f32 dist = vec3_length(hit - center);
            if (fabsf(dist - radius) > thickness) return false;
            if (out_hit) *out_hit = hit;
            return true;
        }

        bool RayAxisHandleHitTest(const Ray& ray, const Vec3& pivot, const Vec3& axis_dir, f32 length, f32 radius, f32* out_distance, f32* out_ray_t, f32* out_axis_t) {
            Vec3 a = pivot;
            Vec3 b = pivot + axis_dir * length;
            f32 distance = 0.0f;
            f32 ray_t = 0.0f;
            f32 seg_t = 0.0f;
            if (!ray_segment_distance(ray, a, b, &distance, &ray_t, &seg_t)) return false;
            if (ray_t < 0.0f) return false;
            if (distance > radius) return false;
            if (out_distance) *out_distance = distance;
            if (out_ray_t) *out_ray_t = ray_t;
            if (out_axis_t) *out_axis_t = seg_t;
            return true;
        }

        bool ray_intersect_aabb(const Ray& ray, const AABB& aabb, f32* out_t) {
            f32 tmin = 0.0f;
            f32 tmax = 1e9f;

            const f32* origin = &ray.origin.x;
            const f32* dir = &ray.dir.x;
            const f32* bmin = &aabb.min.x;
            const f32* bmax = &aabb.max.x;

            for (int i = 0; i < 3; ++i) {
                f32 dir_abs = dir[i] < 0.0f ? -dir[i] : dir[i];
                if (dir_abs < 1e-6f) {
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

        Ray build_ray_for_viewport(const Viewport& viewport, i32 mouse_x, i32 mouse_y) {
            i32 local_x = mouse_x - viewport.rect.x;
            i32 local_y = mouse_y - viewport.rect.y;
            f32 ndc_x = (2.0f * (f32)local_x / (f32)viewport.rect.w) - 1.0f;
            f32 ndc_y = 1.0f - (2.0f * (f32)local_y / (f32)viewport.rect.h);
            f32 aspect = (f32)viewport.rect.w / (f32)viewport.rect.h;
            const Camera& camera = viewport.camera;
            if (viewport.type == ViewportType::Perspective) {
                return RayFromPerspectiveNDC(camera, ndc_x, ndc_y, aspect);
            }
            return RayFromOrthoNDC(camera, ndc_x, ndc_y, aspect, viewport.ortho_size);
        }

        bool ray_segment_distance(const Ray& ray, const Vec3& a, const Vec3& b, f32* out_distance, f32* out_ray_t, f32* out_seg_t) {
            Vec3 u = b - a;
            Vec3 v = ray.dir;
            Vec3 w = a - ray.origin;
            f32 a_dot = vec3_dot(u, u);
            f32 b_dot = vec3_dot(u, v);
            f32 c_dot = vec3_dot(v, v);
            f32 d_dot = vec3_dot(u, w);
            f32 e_dot = vec3_dot(v, w);
            f32 denom = a_dot * c_dot - b_dot * b_dot;
            f32 s = 0.0f;

            if (denom > 1e-6f) {
                s = (b_dot * e_dot - c_dot * d_dot) / denom;
                s = clamp_f32(s, 0.0f, 1.0f);
            }

            f32 t = (e_dot + b_dot * s) / c_dot;
            if (t < 0.0f) {
                t = 0.0f;
                if (a_dot > 1e-6f) {
                    s = clamp_f32(-d_dot / a_dot, 0.0f, 1.0f);
                }
            }

            Vec3 closest_seg = a + u * s;
            Vec3 closest_ray = ray.origin + v * t;
            f32 distance = vec3_length(closest_seg - closest_ray);

            if (out_distance) *out_distance = distance;
            if (out_ray_t) *out_ray_t = t;
            if (out_seg_t) *out_seg_t = s;
            return true;
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

        bool editor_is_transform_ui_id(i32 id) {
            return (id >= kInspectorPositionId + 1 && id <= kInspectorPositionId + 3) ||
                (id >= kInspectorRotationId + 1 && id <= kInspectorRotationId + 3) ||
                (id >= kInspectorScaleId + 1 && id <= kInspectorScaleId + 3);
        }

        bool editor_transform_target_valid(const Scene* scene, SelectionType type, u32 index) {
            switch (type) {
            case SelectionType::Brush:
                return index < scene->brush_count;
            case SelectionType::Prop:
                return index < scene->prop_count;
            case SelectionType::Light:
                return index < scene->lights.point_light_count;
            case SelectionType::None:
            default:
                return false;
            }
        }

        bool editor_get_transform_for_target(const Scene* scene, SelectionType type, u32 index, Transform* out_transform) {
            if (!editor_transform_target_valid(scene, type, index)) return false;
            if (type == SelectionType::Prop) {
                const PropEntity& prop = scene->props[index];
                if (out_transform) *out_transform = prop.transform;
                return true;
            }
            if (type == SelectionType::Brush) {
                const Brush& brush = scene->brushes[index];
                AABB bounds = brush_to_aabb(&brush);
                if (out_transform) {
                    out_transform->position = aabb_center(bounds);
                    out_transform->rotation = quat_identity();
                    out_transform->scale = aabb_half_size(bounds) * 2.0f;
                }
                return true;
            }
            if (type == SelectionType::Light) {
                const PointLight& light = scene->lights.point_lights[index];
                if (out_transform) {
                    out_transform->position = light.position;
                    out_transform->rotation = quat_from_euler_radians(light.rotation);
                    out_transform->scale = light.scale;
                }
                return true;
            }
            return false;
        }

        void editor_set_transform_for_target(EditorState* editor, Scene* scene, SelectionType type, u32 index, const Transform& transform) {
            if (!editor_transform_target_valid(scene, type, index)) return;
            if (type == SelectionType::Prop) {
                PropEntity& prop = scene->props[index];
                prop.transform = transform;
                prop.transform.scale.x = std::max(prop.transform.scale.x, 0.05f);
                prop.transform.scale.y = std::max(prop.transform.scale.y, 0.05f);
                prop.transform.scale.z = std::max(prop.transform.scale.z, 0.05f);
                return;
            }
            if (type == SelectionType::Brush) {
                Brush& brush = scene->brushes[index];
                Vec3 size = transform.scale;
                size.x = std::max(size.x, 0.1f);
                size.y = std::max(size.y, 0.1f);
                size.z = std::max(size.z, 0.1f);
                Vec3 half = size * 0.5f;
                brush.min = transform.position - half;
                brush.max = transform.position + half;
                editor->rebuild_world = true;
                editor->rebuild_collision = true;
                return;
            }
            if (type == SelectionType::Light) {
                PointLight& light = scene->lights.point_lights[index];
                light.position = transform.position;
                light.rotation = quat_to_euler_radians(transform.rotation);
                light.scale = transform.scale;
                return;
            }
        }

        bool editor_get_selected_transform(const EditorState* editor, const Scene* scene, Transform* out_transform) {
            return editor_get_transform_for_target(scene, editor->selection_type, editor->selection_index, out_transform);
        }

        void editor_set_selected_transform(EditorState* editor, Scene* scene, const Transform& transform) {
            editor_set_transform_for_target(editor, scene, editor->selection_type, editor->selection_index, transform);
        }

        Vec3 editor_get_selected_pivot(const EditorState* editor, const Scene* scene) {
            Transform transform;
            if (!editor_get_selected_transform(editor, scene, &transform)) return Vec3(0.0f, 0.0f, 0.0f);
            return transform.position;
        }

        void editor_push_transform_command(EditorState* editor, const TransformCommand& command) {
            editor->undo_stack.push_back(command);
            editor->redo_stack.clear();
        }

        void editor_cancel_inspector_edit(EditorState* editor) {
            editor->inspector_edit_active = false;
            editor->inspector_edit_ui_id = 0;
        }

        void editor_cancel_active_sessions(EditorState* editor) {
            if (editor->gizmo_drag_active) {
                editor->gizmo_drag_active = false;
                editor->gizmo_axis_active = EditorState::GizmoAxis::None;
                editor->gizmo_drag_entity_id = kInvalidEntityId;
                editor->gizmo_drag_type = SelectionType::None;
                editor->gizmo_drag_index = 0;
                editor->gizmo_drag_uniform = false;
            }
            editor_cancel_inspector_edit(editor);
        }

        void editor_finalize_gizmo_drag(EditorState* editor, Scene* scene, bool commit) {
            if (!editor->gizmo_drag_active) return;
            if (commit) {
                Transform after;
                if (editor_get_transform_for_target(scene, editor->gizmo_drag_type, editor->gizmo_drag_index, &after)) {
                    if (!transform_near_equal(editor->gizmo_drag_start_transform, after, kTransformEpsilon)) {
                        TransformCommand cmd = { editor->gizmo_drag_type, editor->gizmo_drag_index,
                            editor->gizmo_drag_start_transform, after };
                        editor_push_transform_command(editor, cmd);
                    }
                }
            }
            editor->gizmo_drag_active = false;
            editor->gizmo_axis_active = EditorState::GizmoAxis::None;
            editor->gizmo_drag_entity_id = kInvalidEntityId;
            editor->gizmo_drag_type = SelectionType::None;
            editor->gizmo_drag_index = 0;
            editor->gizmo_drag_uniform = false;
        }

        void editor_begin_inspector_edit(EditorState* editor, Scene* scene, i32 ui_id) {
            Transform before;
            if (!editor_get_selected_transform(editor, scene, &before)) return;
            editor->inspector_edit_active = true;
            editor->inspector_edit_ui_id = ui_id;
            editor->inspector_edit_type = editor->selection_type;
            editor->inspector_edit_index = editor->selection_index;
            editor->inspector_edit_before = before;
        }

        void editor_end_inspector_edit(EditorState* editor, Scene* scene, bool commit) {
            if (!editor->inspector_edit_active) return;
            if (commit) {
                Transform after;
                if (editor_get_transform_for_target(scene, editor->inspector_edit_type, editor->inspector_edit_index, &after)) {
                    if (!transform_near_equal(editor->inspector_edit_before, after, kTransformEpsilon)) {
                        TransformCommand cmd = { editor->inspector_edit_type, editor->inspector_edit_index,
                            editor->inspector_edit_before, after };
                        editor_push_transform_command(editor, cmd);
                    }
                }
            }
            editor_cancel_inspector_edit(editor);
        }

        void editor_update_inspector_edit_session(EditorState* editor, Scene* scene) {
            i32 active_id = editor->ui_active_id;
            bool active_is_transform = editor_is_transform_ui_id(active_id);
            if (editor->inspector_edit_active) {
                bool selection_changed = editor->inspector_edit_type != editor->selection_type ||
                    editor->inspector_edit_index != editor->selection_index;
                bool same_active = active_is_transform && active_id == editor->inspector_edit_ui_id;
                if (!same_active) {
                    editor_end_inspector_edit(editor, scene, !selection_changed);
                    if (!selection_changed && active_is_transform && active_id != 0) {
                        editor_begin_inspector_edit(editor, scene, active_id);
                    }
                }
            }
            else if (active_is_transform && active_id != 0) {
                editor_begin_inspector_edit(editor, scene, active_id);
            }
        }

        void editor_select_none(EditorState* editor) {
            editor_cancel_active_sessions(editor);
            editor->selection_type = SelectionType::None;
            editor->selection_index = 0;
            editor->selectedEntityId = kInvalidEntityId;
        }

        void editor_select(EditorState* editor, SelectionType type, u32 index) {
            editor_cancel_active_sessions(editor);
            editor->selection_type = type;
            editor->selection_index = index;
            editor->selectedEntityId = pack_entity_id(type, index);
        }

        void editor_apply_snap(EditorState* editor, Vec3* value) {
            if (!editor->snap_enabled) return;
            f32 snap = editor->snap_value;
            value->x = roundf(value->x / snap) * snap;
            value->y = roundf(value->y / snap) * snap;
            value->z = roundf(value->z / snap) * snap;
        }

        Vec3 quat_rotate_vec3(const Quat& q, const Vec3& v) {
            Vec3 qv(q.x, q.y, q.z);
            Vec3 t = vec3_cross(qv, v) * 2.0f;
            return v + t * q.w + vec3_cross(qv, t);
        }

        Quat quat_from_axis_angle(const Vec3& axis, f32 angle) {
            Vec3 n = vec3_normalize(axis);
            f32 half = angle * 0.5f;
            f32 s = sinf(half);
            return quat_normalize({ n.x * s, n.y * s, n.z * s, cosf(half) });
        }

        Vec3 editor_gizmo_axis_dir(EditorState::GizmoAxis axis, const Camera& camera, bool local_space, const Transform* transform) {
            (void)camera;
            Vec3 dir = {};            switch (axis) {
            case EditorState::GizmoAxis::X: dir = Vec3(1.0f, 0.0f, 0.0f); break;
            case EditorState::GizmoAxis::Y: dir = Vec3(0.0f, 1.0f, 0.0f); break;
            case EditorState::GizmoAxis::Z: dir = Vec3(0.0f, 0.0f, 1.0f); break;
            case EditorState::GizmoAxis::Center: dir = Vec3(0.0f, 0.0f, 0.0f); break;
            default: dir = Vec3(0.0f, 0.0f, 0.0f); break;
            }
            if (local_space && transform) {
                dir = quat_rotate_vec3(transform->rotation, dir);
            }
            return dir;
        }

        

        f32 editor_gizmo_scale(const Viewport& viewport, const Vec3& pivot) {
            if (viewport.type != ViewportType::Perspective) {
                f32 scale = viewport.ortho_size * 0.2f;
                return std::max(scale, kGizmoMinScale);
            }
            f32 distance = vec3_length(pivot - viewport.camera.position);
            f32 scale = distance * tanf(viewport.camera.fov * 0.5f) * kGizmoScreenFactor;
            return std::max(scale, kGizmoMinScale);
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
                Vec3 rotation = quat_to_euler_radians(prop.transform.rotation);
                file << "    {\"position\": [" << prop.transform.position.x << ", " << prop.transform.position.y << ", " << prop.transform.position.z << "], ";
                file << "\"rotation\": [" << rotation.x << ", " << rotation.y << ", " << rotation.z << "], ";
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
            Vec3 rotation = Vec3(0.0f, 0.0f, 0.0f);
            while (true) {
                std::string key;
                if (!json_read_string(c, &key)) return false;
                if (!json_expect(c, ':')) return false;
                if (key == "position") {
                    if (!json_read_vec3(c, &prop->transform.position)) return false;
                }
                else if (key == "rotation") {
                    if (!json_read_vec3(c, &rotation)) return false;
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
            prop->transform.rotation = quat_from_euler_radians(rotation);
            return true;
        }

        bool json_read_point_light(JsonCursor* c, PointLight* light) {
            if (!json_expect(c, '{')) return false;
            light->active = true;
            light->rotation = Vec3(0.0f, 0.0f, 0.0f);
            light->scale = Vec3(1.0f, 1.0f, 1.0f);
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

        Rect editor_viewport_area(const PlatformState* platform) {
            i32 left = kPanelPadding + kHierarchyWidth + kPanelPadding;
            i32 right = platform->window_width - kInspectorWidth - kPanelPadding - kPanelPadding;
            i32 top = kPanelPadding;
            i32 bottom = platform->window_height - kAssetsHeight - kLineHeight * 2 - kPanelPadding;
            i32 width = std::max(1, right - left);
            i32 height = std::max(1, bottom - top);
            return { left, top, width, height };
        }

        void editor_build_viewports(EditorState* editor, const PlatformState* platform, Viewport* out_viewports, i32* out_count) {
            Rect area = editor_viewport_area(platform);
            i32 half_w = std::max(1, area.w / 2);
            i32 half_h = std::max(1, area.h / 2);

            Viewport viewports[kEditorViewportCount] = {};

            viewports[0].id = kViewportIdPerspective;
            viewports[0].rect = { area.x, area.y, half_w, half_h };
            viewports[0].camera = editor->camera;
            viewports[0].type = ViewportType::Perspective;
            viewports[0].ortho_size = 0.0f;

            viewports[1].id = kViewportIdTop;
            viewports[1].rect = { area.x + half_w, area.y, area.w - half_w, half_h };
            viewports[1].camera = editor->ortho_top_camera;
            viewports[1].type = ViewportType::OrthoTop;
            viewports[1].ortho_size = editor->ortho_top_zoom;

            viewports[2].id = kViewportIdFront;
            viewports[2].rect = { area.x, area.y + half_h, half_w, area.h - half_h };
            viewports[2].camera = editor->ortho_front_camera;
            viewports[2].type = ViewportType::OrthoFront;
            viewports[2].ortho_size = editor->ortho_front_zoom;

            viewports[3].id = kViewportIdLeft;
            viewports[3].rect = { area.x + half_w, area.y + half_h, area.w - half_w, area.h - half_h };
            viewports[3].camera = editor->ortho_left_camera;
            viewports[3].type = ViewportType::OrthoLeft;
            viewports[3].ortho_size = editor->ortho_left_zoom;

            for (i32 i = 0; i < kEditorViewportCount; ++i) {
                Viewport& viewport = viewports[i];
                viewport.isHovered = mouse_in_rect(&platform->input, viewport.rect.x, viewport.rect.y, viewport.rect.w, viewport.rect.h);
                viewport.isActive = (editor->activeViewportId == viewport.id);
                out_viewports[i] = viewport;
            }
            if (out_count) *out_count = kEditorViewportCount;
        }

        void editor_validate_selection(EditorState* editor, const Scene* scene) {
            bool valid = true;
            if (editor->selection_type == SelectionType::Brush) {
                valid = editor->selection_index < scene->brush_count;
            }
            else if (editor->selection_type == SelectionType::Prop) {
                valid = editor->selection_index < scene->prop_count;
            }
            else if (editor->selection_type == SelectionType::Light) {
                valid = editor->selection_index < scene->lights.point_light_count;
            }
            else {
                valid = editor->selection_type == SelectionType::None;
            }

            if (!valid) {
                editor_select_none(editor);
            }
            else if (editor->selection_type == SelectionType::None) {
                editor->selectedEntityId = kInvalidEntityId;
            }
        }

        void editor_update_selection(EditorState* editor, Scene* scene, PlatformState* platform, const Viewport& viewport) {
            const InputState* input = &platform->input;
            if (!input->mouse.left.pressed) return;

            Ray ray = build_ray_for_viewport(viewport, input->mouse.x, input->mouse.y);

            f32 best_t = 1e9f;
            SelectionType best_type = SelectionType::None;
            u32 best_index = 0;

            for (u32 i = 0; i < scene->brush_count; ++i) {
                f32 t = 0.0f;
                if (ray_intersect_aabb(ray, brush_to_aabb(&scene->brushes[i]), &t) && t < best_t) {
                    best_t = t;
                    best_type = SelectionType::Brush;
                    best_index = i;
                }
            }

            for (u32 i = 0; i < scene->prop_count; ++i) {
                f32 t = 0.0f;
                if (ray_intersect_aabb(ray, prop_aabb(scene->props[i]), &t) && t < best_t) {
                    best_t = t;
                    best_type = SelectionType::Prop;
                    best_index = i;
                }
            }

            for (u32 i = 0; i < scene->lights.point_light_count; ++i) {
                f32 t = 0.0f;
                Vec3 light_size(0.2f, 0.2f, 0.2f);
                AABB light_aabb = aabb_from_center_size(scene->lights.point_lights[i].position, light_size);
                if (ray_intersect_aabb(ray, light_aabb, &t) && t < best_t) {
                    best_t = t;
                    best_type = SelectionType::Light;
                    best_index = i;
                }
            }

            if (best_type == SelectionType::None) {
                editor_select_none(editor);
            }
            else {
                editor_select(editor, best_type, best_index);
            }
        }

        EditorState::GizmoAxis editor_gizmo_pick_axis_translate(const EditorState* editor, const Viewport& viewport, const InputState* input, const Vec3& pivot, f32 scale, const Transform& transform) {
            Ray ray = build_ray_for_viewport(viewport, input->mouse.x, input->mouse.y);
            f32 hit_radius = scale * kGizmoHitFactor;
            f32 best_distance = 1e9f;
            EditorState::GizmoAxis best_axis = EditorState::GizmoAxis::None;

            for (EditorState::GizmoAxis axis : { EditorState::GizmoAxis::X, EditorState::GizmoAxis::Y, EditorState::GizmoAxis::Z }) {
                Vec3 axis_dir = editor_gizmo_axis_dir(axis, viewport.camera, editor->gizmo_local_space, &transform);
                Vec3 a = pivot;
                Vec3 b = pivot + axis_dir * scale;
                f32 distance = 0.0f;
                f32 ray_t = 0.0f;
                if (!ray_segment_distance(ray, a, b, &distance, &ray_t, nullptr)) {
                    continue;
                }
                if (ray_t < 0.0f) continue;
                if (distance <= hit_radius && distance < best_distance) {
                    best_distance = distance;
                    best_axis = axis;
                }
            }
            return best_axis;
        }

        EditorState::GizmoAxis editor_gizmo_pick_axis_rotate(const EditorState* editor, const Viewport& viewport, const InputState* input, const Vec3& pivot, f32 scale, const Transform& transform) {
            Ray ray = build_ray_for_viewport(viewport, input->mouse.x, input->mouse.y);
            f32 thickness = scale * kGizmoHitFactor;
            f32 best_distance = 1e9f;
            EditorState::GizmoAxis best_axis = EditorState::GizmoAxis::None;

            for (EditorState::GizmoAxis axis : { EditorState::GizmoAxis::X, EditorState::GizmoAxis::Y, EditorState::GizmoAxis::Z }) {
                Vec3 axis_dir = editor_gizmo_axis_dir(axis, viewport.camera, editor->gizmo_local_space, &transform);
                Vec3 hit;
                if (!RayRingHitTest(ray, pivot, axis_dir, scale, thickness, &hit)) continue;
                f32 dist = fabsf(vec3_length(hit - pivot) - scale);
                if (dist < best_distance) {
                    best_distance = dist;
                    best_axis = axis;
                }
            }
            return best_axis;
        }

        EditorState::GizmoAxis editor_gizmo_pick_axis_scale(const EditorState* editor, const Viewport& viewport, const InputState* input, const Vec3& pivot, f32 scale, const Transform& transform) {
            Ray ray = build_ray_for_viewport(viewport, input->mouse.x, input->mouse.y);
            f32 hit_radius = scale * kGizmoHitFactor;
            f32 best_distance = 1e9f;
            EditorState::GizmoAxis best_axis = EditorState::GizmoAxis::None;

            Vec3 to_center = pivot - ray.origin;
            f32 center_t = vec3_dot(to_center, ray.dir);
            Vec3 closest = ray.origin + ray.dir * center_t;
            f32 center_distance = vec3_length(closest - pivot);
            if (center_t >= 0.0f && center_distance <= hit_radius) {
                best_axis = EditorState::GizmoAxis::Center;
                best_distance = center_distance;
            }

            for (EditorState::GizmoAxis axis : { EditorState::GizmoAxis::X, EditorState::GizmoAxis::Y, EditorState::GizmoAxis::Z }) {
                Vec3 axis_dir = editor_gizmo_axis_dir(axis, viewport.camera, editor->gizmo_local_space, &transform);
                f32 distance = 0.0f;
                if (!RayAxisHandleHitTest(ray, pivot, axis_dir, scale, hit_radius, &distance, nullptr, nullptr)) continue;
                if (distance < best_distance) {
                    best_distance = distance;
                    best_axis = axis;
                }
            }
            return best_axis;
        }

        bool editor_update_gizmo(EditorState* editor, Scene* scene, PlatformState* platform, const Viewport& viewport) {

            const InputState* input = &platform->input;
            if (editor->gizmo_mode == EditorState::GizmoMode::None) {
                editor->gizmo_axis_hot = EditorState::GizmoAxis::None;
                if (editor->gizmo_drag_active) editor_finalize_gizmo_drag(editor, scene, false);
                return false;
            }

            if (editor->selection_type == SelectionType::None) {
                editor->gizmo_axis_hot = EditorState::GizmoAxis::None;
                if (editor->gizmo_drag_active) editor_finalize_gizmo_drag(editor, scene, false);
                return false;
            }

            Transform transform;
            if (!editor_get_selected_transform(editor, scene, &transform)) {
                editor->gizmo_axis_hot = EditorState::GizmoAxis::None;
                if (editor->gizmo_drag_active) editor_finalize_gizmo_drag(editor, scene, false);
                return false;
            }
            Vec3 pivot = transform.position;

            f32 scale = editor_gizmo_scale(viewport, pivot);
            bool ui_capture = mouse_over_ui(platform);
            bool shift_down = platform_key_down(input, KEY_SHIFT);

            if (editor->gizmo_drag_active) {
                if (editor->selectedEntityId != editor->gizmo_drag_entity_id) {
                    editor_finalize_gizmo_drag(editor, scene, false);
                    return false;
                }
                if (!input->mouse.left.down) {
                    editor_finalize_gizmo_drag(editor, scene, true);
                    return false;
                }
                if (!platform->input_focused || ui_capture || editor->selection_type == SelectionType::None) {
                    editor_finalize_gizmo_drag(editor, scene, false);
                    return false;
                }
                Ray current_ray = build_ray_for_viewport(viewport, input->mouse.x, input->mouse.y);
                Plane plane = plane_from_point_normal(editor->gizmo_drag_plane_point, editor->gizmo_drag_plane_normal);
                Vec3 current_hit;
                if (!RayPlaneIntersect(current_ray, plane, nullptr, &current_hit)) {
                    return true;
                }

                if (editor->gizmo_mode == EditorState::GizmoMode::Translate) {
                    Vec3 delta = current_hit - editor->gizmo_drag_start_hit;
                    f32 axis_delta = vec3_dot(delta, editor->gizmo_drag_axis);
                    if (editor->snap_enabled && editor->snap_value > 0.0001f) {
                        axis_delta = roundf(axis_delta / editor->snap_value) * editor->snap_value;
                    }
                    Vec3 new_pos = editor->gizmo_drag_start_pos + editor->gizmo_drag_axis * axis_delta;
                    Transform updated = editor->gizmo_drag_start_transform;
                    updated.position = new_pos;
                    editor_set_selected_transform(editor, scene, updated);
                    return true;
                }

                if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) {
                    Vec3 current_vec = vec3_normalize(current_hit - pivot);
                    Vec3 start_vec = editor->gizmo_drag_start_vector;
                    Vec3 axis = editor->gizmo_drag_axis;
                    f32 angle = atan2f(vec3_dot(axis, vec3_cross(start_vec, current_vec)), vec3_dot(start_vec, current_vec));
                    f32 snap = editor->rotate_snap_value;
                    if ((editor->rotate_snap_enabled || shift_down) && snap > 0.0001f) {
                        angle = roundf(angle / degrees_to_radians(snap)) * degrees_to_radians(snap);
                    }
                    Quat delta_q = quat_from_axis_angle(axis, angle);
                    Transform updated = editor->gizmo_drag_start_transform;
                    updated.rotation = quat_multiply(delta_q, editor->gizmo_drag_start_transform.rotation);
                    editor_set_selected_transform(editor, scene, updated);
                    return true;
                }

                if (editor->gizmo_mode == EditorState::GizmoMode::Scale) {
                    Vec3 delta = current_hit - editor->gizmo_drag_start_hit;
                    f32 axis_delta = vec3_dot(delta, editor->gizmo_drag_axis);
                    f32 factor = 1.0f + axis_delta / std::max(scale, 0.0001f);
                    if (editor->scale_snap_enabled && editor->scale_snap_value > 0.0001f) {
                        factor = roundf(factor / editor->scale_snap_value) * editor->scale_snap_value;
                    }
                    factor = std::max(factor, 0.01f);
                    Transform updated = editor->gizmo_drag_start_transform;
                    if (editor->gizmo_drag_uniform || editor->gizmo_axis_active == EditorState::GizmoAxis::Center) {
                        updated.scale = editor->gizmo_drag_start_transform.scale * factor;
                    }
                    else {
                        Vec3 new_scale = editor->gizmo_drag_start_transform.scale;
                        if (editor->gizmo_axis_active == EditorState::GizmoAxis::X) new_scale.x *= factor;
                        if (editor->gizmo_axis_active == EditorState::GizmoAxis::Y) new_scale.y *= factor;
                        if (editor->gizmo_axis_active == EditorState::GizmoAxis::Z) new_scale.z *= factor;
                        updated.scale = new_scale;
                    }
                    editor_set_selected_transform(editor, scene, updated);
                    return true;
                }
                
                return true;
            }
            if (!platform->input_focused || ui_capture || platform->mouse_captured) {
                editor->gizmo_axis_hot = EditorState::GizmoAxis::None;
                return false;
            }

            if (!viewport.isHovered) {
                editor->gizmo_axis_hot = EditorState::GizmoAxis::None;
                return false;
            }

            if (editor->gizmo_mode == EditorState::GizmoMode::Translate) {
                editor->gizmo_axis_hot = editor_gizmo_pick_axis_translate(editor, viewport, input, pivot, scale, transform);
            }
            else if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) {
                editor->gizmo_axis_hot = editor_gizmo_pick_axis_rotate(editor, viewport, input, pivot, scale, transform);
            }
            else if (editor->gizmo_mode == EditorState::GizmoMode::Scale) {
                editor->gizmo_axis_hot = editor_gizmo_pick_axis_scale(editor, viewport, input, pivot, scale, transform);
            }

            if (input->mouse.left.pressed && editor->gizmo_axis_hot != EditorState::GizmoAxis::None && viewport.isHovered) {
                editor->gizmo_drag_active = true;
                editor->gizmo_axis_active = editor->gizmo_axis_hot;
                editor->gizmo_drag_entity_id = editor->selectedEntityId;
                editor->gizmo_drag_start_pos = pivot;
                editor->gizmo_drag_start_transform = transform;
                editor->gizmo_drag_type = editor->selection_type;
                editor->gizmo_drag_index = editor->selection_index;
                editor->gizmo_drag_uniform = (editor->gizmo_axis_hot == EditorState::GizmoAxis::Center);

                Ray start_ray = build_ray_for_viewport(viewport, input->mouse.x, input->mouse.y);
                Vec3 view_dir = start_ray.dir;
                if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) {
                    editor->gizmo_drag_axis = vec3_normalize(editor_gizmo_axis_dir(editor->gizmo_axis_active, viewport.camera, editor->gizmo_local_space, &transform));
                    editor->gizmo_drag_plane_normal = editor->gizmo_drag_axis;
                    editor->gizmo_drag_plane_point = pivot;
                    Plane plane = plane_from_point_normal(editor->gizmo_drag_plane_point, editor->gizmo_drag_plane_normal);
                    Vec3 hit_point = pivot;
                    if (!ray_plane_intersect(start_ray, plane, nullptr, &hit_point)) {
                        editor_finalize_gizmo_drag(editor, scene, false);
                        return false;
                    }
                    editor->gizmo_drag_start_hit = hit_point;
                    if (editor->gizmo_mode == EditorState::GizmoMode::Scale && editor->gizmo_drag_uniform) {
                        Vec3 cam_right = camera_right(&viewport.camera);
                        Vec3 cam_up = vec3_cross(cam_right, camera_forward(&viewport.camera));
                        editor->gizmo_drag_axis = vec3_normalize(cam_right + cam_up);
                    }
                    else {
                        editor->gizmo_drag_axis = vec3_normalize(axis_dir);
                    }
                    editor->gizmo_drag_start_vector = vec3_normalize(hit_point - pivot);
                    return true;
                }

                Vec3 axis_dir = editor_gizmo_axis_dir(editor->gizmo_axis_active, viewport.camera, editor->gizmo_local_space, &transform);
                Vec3 normal = view_dir - axis_dir * vec3_dot(view_dir, axis_dir);
                if (vec3_length(normal) < 1e-4f) {
                    Vec3 up = vec3_cross(camera_right(&viewport.camera), camera_forward(&viewport.camera));
                    normal = vec3_cross(axis_dir, up);
                }
                if (vec3_length(normal) < 1e-4f) {
                    normal = Vec3(0.0f, 1.0f, 0.0f);
                }
                editor->gizmo_drag_plane_normal = vec3_normalize(normal);
                editor->gizmo_drag_plane_point = pivot;

                Plane plane = plane_from_point_normal(editor->gizmo_drag_plane_point, editor->gizmo_drag_plane_normal);
                Vec3 hit_point = pivot;
                ray_plane_intersect(start_ray, plane, nullptr, &hit_point);
                editor->gizmo_drag_start_hit = hit_point;
                return true;
            }

            return false;
        }

        void editor_draw_circle(const Vec3& center, const Vec3& axis, f32 radius, const Vec3& color) {
            Vec3 n = vec3_normalize(axis);
            Vec3 ref = fabsf(n.y) < 0.9f ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
            Vec3 tangent = vec3_normalize(vec3_cross(n, ref));
            Vec3 bitangent = vec3_cross(n, tangent);
            const i32 segments = 48;
            Vec3 prev = center + tangent * radius;
            for (i32 i = 1; i <= segments; ++i) {
                f32 angle = (f32)i / (f32)segments * 6.283185307f;
                Vec3 next = center + (tangent * cosf(angle) + bitangent * sinf(angle)) * radius;
                debug_line(prev, next, color);
                prev = next;
            }
        }

        void editor_draw_axis_handle(const Vec3& pivot, const Vec3& axis_dir, f32 length, f32 size, const Vec3& color) {
            Vec3 dir = vec3_normalize(axis_dir);
            Vec3 ref = fabsf(dir.y) < 0.9f ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
            Vec3 side = vec3_normalize(vec3_cross(dir, ref));
            Vec3 up = vec3_cross(side, dir);
            Vec3 end = pivot + dir * length;
            Vec3 a = end + side * size + up * size;
            Vec3 b = end + side * size - up * size;
            Vec3 c = end - side * size - up * size;
            Vec3 d = end - side * size + up * size;
            debug_line(pivot, end, color);
            debug_line(a, b, color);
            debug_line(b, c, color);
            debug_line(c, d, color);
            debug_line(d, a, color);
        }

        void editor_draw_gizmo_internal(const EditorState* editor, const Scene* scene, const Viewport& viewport) {
            if (editor->gizmo_mode == EditorState::GizmoMode::None) return;
            if (editor->selection_type == SelectionType::None) return;

            Transform transform;
            if (!editor_get_selected_transform(editor, scene, &transform)) return;

            Vec3 pivot = transform.position;

            f32 scale = editor_gizmo_scale(viewport, pivot);

            auto axis_color = [&](EditorState::GizmoAxis axis, const Vec3& base) {
                bool active = (editor->gizmo_axis_active == axis) || (editor->gizmo_drag_active && editor->gizmo_axis_active == axis);
                bool hot = (editor->gizmo_axis_hot == axis);
                if (active) return Vec3(1.0f, 1.0f, 0.3f);
                if (hot) return Vec3(std::min(base.x + 0.3f, 1.0f), std::min(base.y + 0.3f, 1.0f), std::min(base.z + 0.3f, 1.0f));
                return base;
                };

            Vec3 x_dir = editor_gizmo_axis_dir(EditorState::GizmoAxis::X, viewport.camera, editor->gizmo_local_space, &transform);
            Vec3 y_dir = editor_gizmo_axis_dir(EditorState::GizmoAxis::Y, viewport.camera, editor->gizmo_local_space, &transform);
            Vec3 z_dir = editor_gizmo_axis_dir(EditorState::GizmoAxis::Z, viewport.camera, editor->gizmo_local_space, &transform);

            if (editor->gizmo_mode == EditorState::GizmoMode::Translate) {
                debug_line(pivot, pivot + x_dir * scale, axis_color(EditorState::GizmoAxis::X, Vec3(0.9f, 0.1f, 0.1f)));
                debug_line(pivot, pivot + y_dir * scale, axis_color(EditorState::GizmoAxis::Y, Vec3(0.1f, 0.9f, 0.1f)));
                debug_line(pivot, pivot + z_dir * scale, axis_color(EditorState::GizmoAxis::Z, Vec3(0.2f, 0.3f, 0.9f)));

                f32 tip = scale * 0.15f;
                debug_line(pivot + x_dir * scale, pivot + x_dir * (scale - tip) + y_dir * tip * 0.3f, axis_color(EditorState::GizmoAxis::X, Vec3(0.9f, 0.1f, 0.1f)));
                debug_line(pivot + y_dir * scale, pivot + y_dir * (scale - tip) + z_dir * tip * 0.3f, axis_color(EditorState::GizmoAxis::Y, Vec3(0.1f, 0.9f, 0.1f)));
                debug_line(pivot + z_dir * scale, pivot + z_dir * (scale - tip) + x_dir * tip * 0.3f, axis_color(EditorState::GizmoAxis::Z, Vec3(0.2f, 0.3f, 0.9f)));
            }
            else if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) {
                editor_draw_circle(pivot, x_dir, scale, axis_color(EditorState::GizmoAxis::X, Vec3(0.9f, 0.1f, 0.1f)));
                editor_draw_circle(pivot, y_dir, scale, axis_color(EditorState::GizmoAxis::Y, Vec3(0.1f, 0.9f, 0.1f)));
                editor_draw_circle(pivot, z_dir, scale, axis_color(EditorState::GizmoAxis::Z, Vec3(0.2f, 0.3f, 0.9f)));
            }
            else if (editor->gizmo_mode == EditorState::GizmoMode::Scale) {
                f32 handle = scale * 0.1f;
                editor_draw_axis_handle(pivot, x_dir, scale, handle, axis_color(EditorState::GizmoAxis::X, Vec3(0.9f, 0.1f, 0.1f)));
                editor_draw_axis_handle(pivot, y_dir, scale, handle, axis_color(EditorState::GizmoAxis::Y, Vec3(0.1f, 0.9f, 0.1f)));
                editor_draw_axis_handle(pivot, z_dir, scale, handle, axis_color(EditorState::GizmoAxis::Z, Vec3(0.2f, 0.3f, 0.9f)));
                Vec3 center_color = axis_color(EditorState::GizmoAxis::Center, Vec3(0.9f, 0.9f, 0.9f));
                debug_wire_box(pivot, Vec3(handle * 1.5f, handle * 1.5f, handle * 1.5f), center_color);
            }
        }

        void editor_draw_selection_bounds_internal(const EditorState* editor, const Scene* scene) {
            if (editor->selection_type == SelectionType::Brush) {
                const Brush& brush = scene->brushes[editor->selection_index];
                AABB brush_aabb = brush_to_aabb(&brush);
                debug_wire_box(aabb_center(brush_aabb), aabb_half_size(brush_aabb) * 2.0f, Vec3(1, 0.8f, 0.2f));
            }
            else if (editor->selection_type == SelectionType::Prop) {
                const PropEntity& prop = scene->props[editor->selection_index];
                debug_wire_box(prop.transform.position, prop.transform.scale, Vec3(0.3f, 1.0f, 0.5f));
            }
            else if (editor->selection_type == SelectionType::Light) {
                const PointLight& light = scene->lights.point_lights[editor->selection_index];
                debug_wire_box(light.position, Vec3(0.2f, 0.2f, 0.2f), Vec3(1.0f, 0.9f, 0.4f));
            }
        }

        void editor_draw_grid_plane(const Vec3& origin, const Vec3& axis_a, const Vec3& axis_b, f32 extent, f32 spacing, i32 major_step) {
            i32 count = (i32)std::ceil(extent / spacing);
            for (i32 i = -count; i <= count; ++i) {
                f32 offset = (f32)i * spacing;
                Vec3 color = (i % major_step == 0) ? Vec3(0.4f, 0.4f, 0.45f) : Vec3(0.25f, 0.25f, 0.3f);
                Vec3 a0 = origin + axis_a * (-extent) + axis_b * offset;
                Vec3 a1 = origin + axis_a * extent + axis_b * offset;
                Vec3 b0 = origin + axis_b * (-extent) + axis_a * offset;
                Vec3 b1 = origin + axis_b * extent + axis_a * offset;
                debug_line(a0, a1, color);
                debug_line(b0, b1, color);
            }
        }

        void editor_draw_ortho_grid_internal(const EditorState* editor, const Viewport& viewport) {
            if (!editor->show_grid) return;
            f32 extent = std::max(viewport.ortho_size * 1.2f, 1.0f);
            f32 spacing = 1.0f;
            if (extent > 20.0f) spacing = 2.0f;
            if (extent > 50.0f) spacing = 5.0f;
            if (extent > 100.0f) spacing = 10.0f;
            i32 major_step = 5;
            Vec3 origin(0.0f, 0.0f, 0.0f);
            if (viewport.type == ViewportType::OrthoTop) {
                editor_draw_grid_plane(origin, Vec3(1, 0, 0), Vec3(0, 0, 1), extent, spacing, major_step);
            }
            else if (viewport.type == ViewportType::OrthoFront) {
                editor_draw_grid_plane(origin, Vec3(1, 0, 0), Vec3(0, 1, 0), extent, spacing, major_step);
            }
            else if (viewport.type == ViewportType::OrthoLeft) {
                editor_draw_grid_plane(origin, Vec3(0, 0, 1), Vec3(0, 1, 0), extent, spacing, major_step);
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
                    editor_select(editor, SelectionType::Brush, i);
                }
                y += kLineHeight;
            }

            for (u32 i = 0; i < scene->prop_count; ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Prop %u", i);
                bool clicked = ui_button(editor, input, 2000 + (i32)i, x, y, kHierarchyWidth, label);
                if (clicked) {
                    editor_select(editor, SelectionType::Prop, i);
                }
                y += kLineHeight;
            }

            for (u32 i = 0; i < scene->lights.point_light_count; ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Light %u", i);
                bool clicked = ui_button(editor, input, 3000 + (i32)i, x, y, kHierarchyWidth, label);
                if (clicked) {
                    editor_select(editor, SelectionType::Light, i);
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

            if (editor->selection_type == SelectionType::None) {
                debug_text_printf(x, y, Vec3(0.7f, 0.7f, 0.7f), "No selection");
                return;
            }

            const char* type_text = editor->selection_type == SelectionType::Brush ? "Brush"
                : (editor->selection_type == SelectionType::Prop ? "Prop" : "Light");
            debug_text_printf(x, y, Vec3(0.7f, 0.7f, 0.7f), "Type: %s", type_text);
            y += kLineHeight;
            debug_text_printf(x, y, Vec3(0.7f, 0.7f, 0.7f), "Name: %s %u", type_text, editor->selection_index);
            y += kLineHeight;
            debug_text_printf(x, y, Vec3(0.7f, 0.7f, 0.7f), "Id: %d", editor->selectedEntityId);
            y += kLineHeight;

            Transform transform;
            if (!editor_get_selected_transform(editor, scene, &transform)) {
                return;
            }

            Vec3 rotation_degrees = radians_to_degrees(quat_to_euler_radians(transform.rotation));

            bool changed = false;
            bool position_changed = ui_drag_vec3(editor, input, kInspectorPositionId, x, y, "Position", &transform.position, kDragSpeed);
            changed |= position_changed;
            y += kLineHeight * 4;
            if (ui_button(editor, input, kInspectorResetPositionId, x, y, 60, "Reset")) {
                Transform before = transform;
                transform.position = Vec3(0.0f, 0.0f, 0.0f);
                editor_set_selected_transform(editor, scene, transform);
                Transform after;
                if (editor_get_selected_transform(editor, scene, &after) &&
                    !transform_near_equal(before, after, kTransformEpsilon)) {
                    TransformCommand cmd = { editor->selection_type, editor->selection_index, before, after };
                    editor_push_transform_command(editor, cmd);
                }
                
            }
            y += kLineHeight;

            bool rotation_changed = ui_drag_vec3(editor, input, kInspectorRotationId, x, y, "Rotation (deg)", &rotation_degrees, 0.5f);
            y += kLineHeight * 4;
            if (ui_button(editor, input, kInspectorResetRotationId, x, y, 60, "Reset")) {
                Transform before = transform;
                rotation_degrees = Vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = quat_identity();
                editor_set_selected_transform(editor, scene, transform);
                Transform after;
                if (editor_get_selected_transform(editor, scene, &after) &&
                    !transform_near_equal(before, after, kTransformEpsilon)) {
                    TransformCommand cmd = { editor->selection_type, editor->selection_index, before, after };
                    editor_push_transform_command(editor, cmd);
                }
            }
            y += kLineHeight;

            changed |= rotation_changed;

            bool scale_changed = ui_drag_vec3(editor, input, kInspectorScaleId, x, y, "Scale", &transform.scale, kDragSpeed);
            changed |= scale_changed;
            y += kLineHeight * 4;
            if (ui_button(editor, input, kInspectorResetScaleId, x, y, 60, "Reset")) {
                Transform before = transform;
                transform.scale = Vec3(1.0f, 1.0f, 1.0f);
                editor_set_selected_transform(editor, scene, transform);
                Transform after;
                if (editor_get_selected_transform(editor, scene, &after) &&
                    !transform_near_equal(before, after, kTransformEpsilon)) {
                    TransformCommand cmd = { editor->selection_type, editor->selection_index, before, after };
                    editor_push_transform_command(editor, cmd);
                }
            }
            y += kLineHeight;

            if (rotation_changed) {
                transform.rotation = quat_from_euler_radians(degrees_to_radians(rotation_degrees));
            }

            if (changed) {
                if (position_changed && editor->snap_enabled) {
                    editor_apply_snap(editor, &transform.position);
                }
                if (rotation_changed && editor->rotate_snap_enabled && editor->rotate_snap_value > 0.0001f) {
                    rotation_degrees.x = roundf(rotation_degrees.x / editor->rotate_snap_value) * editor->rotate_snap_value;
                    rotation_degrees.y = roundf(rotation_degrees.y / editor->rotate_snap_value) * editor->rotate_snap_value;
                    rotation_degrees.z = roundf(rotation_degrees.z / editor->rotate_snap_value) * editor->rotate_snap_value;
                    transform.rotation = quat_from_euler_radians(degrees_to_radians(rotation_degrees));
                }
                if (scale_changed && editor->scale_snap_enabled && editor->scale_snap_value > 0.0001f) {
                    transform.scale.x = roundf(transform.scale.x / editor->scale_snap_value) * editor->scale_snap_value;
                    transform.scale.y = roundf(transform.scale.y / editor->scale_snap_value) * editor->scale_snap_value;
                    transform.scale.z = roundf(transform.scale.z / editor->scale_snap_value) * editor->scale_snap_value;
                }
                editor_set_selected_transform(editor, scene, transform);
            }

            const char* mode = "None (Q)";
            if (editor->gizmo_mode == EditorState::GizmoMode::Translate) mode = "Translate (W)";
            else if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) mode = "Rotate (E)";
            else if (editor->gizmo_mode == EditorState::GizmoMode::Scale) mode = "Scale (R)";
            debug_text_printf(x, y, Vec3(0.7f, 0.9f, 0.7f), "Mode: %s", mode);
            y += kLineHeight;

            bool snap = editor->snap_enabled;
            if (ui_checkbox(editor, input, kInspectorSnapTranslateId, x, y, "Snap (Translate)", &snap)) {
                editor->snap_enabled = snap;
            }
            y += kLineHeight;
            ui_drag_float(editor, input, kInspectorSnapTranslateValueId, x, y, "Snap Value", &editor->snap_value, 0.01f);
            y += kLineHeight;
            bool rotate_snap = editor->rotate_snap_enabled;
            if (ui_checkbox(editor, input, kInspectorSnapRotateId, x, y, "Snap (Rotate)", &rotate_snap)) {
                editor->rotate_snap_enabled = rotate_snap;
            }
            y += kLineHeight;
            ui_drag_float(editor, input, kInspectorSnapRotateValueId, x, y, "Rotate Snap", &editor->rotate_snap_value, 1.0f);
            y += kLineHeight;
            bool scale_snap = editor->scale_snap_enabled;
            if (ui_checkbox(editor, input, kInspectorSnapScaleId, x, y, "Snap (Scale)", &scale_snap)) {
                editor->scale_snap_enabled = scale_snap;
            }
            y += kLineHeight;
            ui_drag_float(editor, input, kInspectorSnapScaleValueId, x, y, "Scale Snap", &editor->scale_snap_value, 0.1f);
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
        camera_init(&editor->ortho_top_camera);
        editor->ortho_top_camera.position = Vec3(0.0f, 10.0f, 0.0f);
        editor->ortho_top_camera.pitch = -1.553f;
        editor->ortho_top_camera.yaw = 0.0f;
        camera_init(&editor->ortho_front_camera);
        editor->ortho_front_camera.position = Vec3(0.0f, 2.0f, 10.0f);
        editor->ortho_front_camera.pitch = 0.0f;
        editor->ortho_front_camera.yaw = 0.0f;
        camera_init(&editor->ortho_left_camera);
        editor->ortho_left_camera.position = Vec3(10.0f, 2.0f, 0.0f);
        editor->ortho_left_camera.pitch = 0.0f;
        editor->ortho_left_camera.yaw = -1.570796327f;
        editor->ortho_top_zoom = 10.0f;
        editor->ortho_front_zoom = 10.0f;
        editor->ortho_left_zoom = 10.0f;
        editor->move_speed = 6.0f;
        editor->look_sensitivity = 0.0025f;
        editor->selection_type = SelectionType::None;
        editor->selection_index = 0;
        editor->selectedEntityId = kInvalidEntityId;
        editor->gizmo_mode = EditorState::GizmoMode::Translate;
        editor->snap_enabled = false;
        editor->snap_value = 0.5f;
        editor->rotate_snap_enabled = false;
        editor->rotate_snap_value = 15.0f;
        editor->scale_snap_enabled = false;
        editor->scale_snap_value = 0.1f;
        editor->show_grid = true;
        editor->gizmo_axis_hot = EditorState::GizmoAxis::None;
        editor->gizmo_axis_active = EditorState::GizmoAxis::None;
        editor->gizmo_drag_active = false;
        editor->gizmo_drag_start_pos = Vec3(0, 0, 0);
        editor->gizmo_drag_plane_normal = Vec3(0, 1, 0);
        editor->gizmo_drag_plane_point = Vec3(0, 0, 0);
        editor->gizmo_drag_start_hit = Vec3(0, 0, 0);
        editor->gizmo_drag_axis = Vec3(1, 0, 0);
        editor->gizmo_drag_start_vector = Vec3(1, 0, 0);
        editor->gizmo_drag_start_axis_value = 0.0f;
        editor->gizmo_drag_uniform = false;
        editor->gizmo_drag_start_transform = transform_default();
        editor->gizmo_drag_entity_id = kInvalidEntityId;
        editor->gizmo_drag_type = SelectionType::None;
        editor->gizmo_drag_index = 0;
        editor->gizmo_local_space = false;
        editor->undo_stack.clear();
        editor->redo_stack.clear();
        editor->inspector_edit_active = false;
        editor->inspector_edit_ui_id = 0;
        editor->inspector_edit_type = SelectionType::None;
        editor->inspector_edit_index = 0;
        editor->inspector_edit_before = transform_default();
        std::snprintf(editor->scene_path, sizeof(editor->scene_path), "scene.json");
        editor->rebuild_world = false;
        editor->rebuild_collision = false;
        editor->activeViewportId = kViewportIdPerspective;
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
        editor_validate_selection(editor, scene);
        Viewport viewports[kEditorViewportCount] = {};
        i32 viewport_count = 0;
        editor_build_viewports(editor, platform, viewports, &viewport_count);

        if (platform_key_pressed(input, KEY_Q)) {
            editor->gizmo_mode = EditorState::GizmoMode::None;
            editor_finalize_gizmo_drag(editor, scene, false);
        }
        if (platform_key_pressed(input, KEY_W)) {
            editor->gizmo_mode = EditorState::GizmoMode::Translate;
        }
        if (platform_key_pressed(input, KEY_E)) {
            editor->gizmo_mode = EditorState::GizmoMode::Rotate;
        }
        if (platform_key_pressed(input, KEY_R)) {
            editor->gizmo_mode = EditorState::GizmoMode::Scale;
        }
        if (platform_key_pressed(input, KEY_G)) {
            editor->snap_enabled = !editor->snap_enabled;
        }
        if (platform_key_pressed(input, KEY_L)) {
            editor->gizmo_local_space = !editor->gizmo_local_space;
        }

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

        bool shift_down = platform_key_down(input, KEY_SHIFT);
        if (ctrl_down && platform_key_pressed(input, KEY_Z)) {
            if (shift_down) {
                if (!editor->redo_stack.empty()) {
                    TransformCommand cmd = editor->redo_stack.back();
                    editor->redo_stack.pop_back();
                    if (editor_transform_target_valid(scene, cmd.type, cmd.index)) {
                        editor_set_transform_for_target(editor, scene, cmd.type, cmd.index, cmd.after);
                        editor->undo_stack.push_back(cmd);
                    }
                }
            }
            else if (!editor->undo_stack.empty()) {
                TransformCommand cmd = editor->undo_stack.back();
                editor->undo_stack.pop_back();
                if (editor_transform_target_valid(scene, cmd.type, cmd.index)) {
                    editor_set_transform_for_target(editor, scene, cmd.type, cmd.index, cmd.before);
                    editor->redo_stack.push_back(cmd);
                }
            }
        }
        if (ctrl_down && platform_key_pressed(input, KEY_Y)) {
            if (!editor->redo_stack.empty()) {
                TransformCommand cmd = editor->redo_stack.back();
                editor->redo_stack.pop_back();
                if (editor_transform_target_valid(scene, cmd.type, cmd.index)) {
                    editor_set_transform_for_target(editor, scene, cmd.type, cmd.index, cmd.after);
                    editor->undo_stack.push_back(cmd);
                }
            }
        }

        if (input->mouse.left.pressed && !mouse_over_ui(platform)) {
            for (i32 i = 0; i < viewport_count; ++i) {
                if (viewports[i].isHovered) {
                    editor->activeViewportId = viewports[i].id;
                    break;
                }
            }
        }

        Viewport* active_viewport = nullptr;
        for (i32 i = 0; i < viewport_count; ++i) {
            viewports[i].isActive = (viewports[i].id == editor->activeViewportId);
            if (viewports[i].id == editor->activeViewportId) {
                active_viewport = &viewports[i];
            }
        }
        if (!active_viewport) {
            editor->activeViewportId = kViewportIdPerspective;
            active_viewport = &viewports[0];
            for (i32 i = 0; i < viewport_count; ++i) {
                viewports[i].isActive = (viewports[i].id == editor->activeViewportId);
            }
        }

        bool ui_capture = mouse_over_ui(platform);
        if (active_viewport->type != ViewportType::Perspective && platform->mouse_captured) {
            platform_set_mouse_capture(platform, false);
        }

        if (active_viewport->type == ViewportType::Perspective) {
            if (active_viewport->isHovered && input->mouse.right.pressed && !ui_capture) {
                platform_set_mouse_capture(platform, true);
            }
            if (input->mouse.right.released) {
                platform_set_mouse_capture(platform, false);
            }

            if (platform->mouse_captured) {
                f32 dyaw = (f32)input->mouse.delta_x * editor->look_sensitivity;
                f32 dpitch = (f32)(-input->mouse.delta_y) * editor->look_sensitivity;
                camera_rotate(&editor->camera, dyaw, dpitch);
            }            Vec3 forward = camera_forward(&editor->camera);
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
            active_viewport->camera = editor->camera;
        }

        else {
            Camera* ortho_camera = nullptr;
            f32* ortho_zoom = nullptr;
            if (active_viewport->type == ViewportType::OrthoTop) {
                ortho_camera = &editor->ortho_top_camera;
                ortho_zoom = &editor->ortho_top_zoom;
            }
            else if (active_viewport->type == ViewportType::OrthoFront) {
                ortho_camera = &editor->ortho_front_camera;
                ortho_zoom = &editor->ortho_front_zoom;
            }
            else if (active_viewport->type == ViewportType::OrthoLeft) {
                ortho_camera = &editor->ortho_left_camera;
                ortho_zoom = &editor->ortho_left_zoom;
            }

            if (ortho_camera && ortho_zoom) {
                bool pan_active = (input->mouse.middle.down || input->mouse.right.down);
                if (active_viewport->isHovered && pan_active && !ui_capture && platform->input_focused) {
                    Vec3 right = camera_right(ortho_camera);
                    Vec3 up = vec3_cross(right, camera_forward(ortho_camera));
                    f32 pan_speed = std::max(*ortho_zoom * 0.002f, 0.001f);
                    ortho_camera->position = ortho_camera->position - right * ((f32)input->mouse.delta_x * pan_speed);
                    ortho_camera->position = ortho_camera->position + up * ((f32)input->mouse.delta_y * pan_speed);
                }
                if (active_viewport->isHovered && input->mouse.wheel_delta != 0) {
                    f32 zoom_factor = powf(0.9f, (f32)input->mouse.wheel_delta);
                    *ortho_zoom = clamp_f32((*ortho_zoom) * zoom_factor, 0.5f, 200.0f);
                }
                active_viewport->camera = *ortho_camera;
                active_viewport->ortho_size = *ortho_zoom;
            }
        }

        if (editor->gizmo_drag_active && !platform->input_focused) {
            editor_finalize_gizmo_drag(editor, scene, false);
        }

        bool gizmo_consumed = false;
        if (active_viewport->isActive) {
            gizmo_consumed = editor_update_gizmo(editor, scene, platform, *active_viewport);
        }

        if (active_viewport->isActive && !platform->mouse_captured && input->mouse.left.pressed && active_viewport->isHovered && !ui_capture) {
            if (!gizmo_consumed) {
                editor_update_selection(editor, scene, platform, *active_viewport);
            }
        }
    }

    void editor_draw_ui(EditorState* editor, Scene* scene, PlatformState* platform) {
        ui_begin(editor);
        editor_draw_hierarchy(editor, scene, platform);
        editor_draw_inspector(editor, scene, platform);
        editor_draw_assets(editor, scene, platform);
        editor_draw_top_bar(editor, scene, platform);
        ui_end(editor, &platform->input);
        editor_update_inspector_edit_session(editor, scene);

        debug_text_printf(kPanelPadding, platform->window_height - kAssetsHeight - kLineHeight * 3,
            Vec3(0.7f, 0.7f, 0.7f), "Viewport: %d  Selected: %d", editor->activeViewportId, editor->selectedEntityId);
        const char* mode_text = "None";
        if (editor->gizmo_mode == EditorState::GizmoMode::Translate) mode_text = "Translate";
        else if (editor->gizmo_mode == EditorState::GizmoMode::Rotate) mode_text = "Rotate";
        else if (editor->gizmo_mode == EditorState::GizmoMode::Scale) mode_text = "Scale";
        const char* axis_text = "None";
        if (editor->gizmo_axis_hot == EditorState::GizmoAxis::X) axis_text = "X";
        else if (editor->gizmo_axis_hot == EditorState::GizmoAxis::Y) axis_text = "Y";
        else if (editor->gizmo_axis_hot == EditorState::GizmoAxis::Z) axis_text = "Z";
        else if (editor->gizmo_axis_hot == EditorState::GizmoAxis::Center) axis_text = "Center";
        debug_text_printf(kPanelPadding, platform->window_height - kAssetsHeight - kLineHeight * 2,
            Vec3(0.7f, 0.7f, 0.7f), "Gizmo: %s  AxisHot: %s  Drag: %s", mode_text, axis_text, editor->gizmo_drag_active ? "On" : "Off");
        debug_text_printf(kPanelPadding, platform->window_height - kAssetsHeight - kLineHeight,
            Vec3(0.7f, 0.7f, 0.7f), "Snap: %s  Step: %.2f", editor->snap_enabled ? "On" : "Off", editor->snap_value);
    }

    void editor_get_viewports(EditorState* editor, const PlatformState* platform, Viewport* out_viewports, i32* out_count) {
        editor_build_viewports(editor, platform, out_viewports, out_count);
    }

    void editor_draw_gizmo(const EditorState* editor, const Scene* scene, const Viewport& viewport) {
        editor_draw_gizmo_internal(editor, scene, viewport);
    }

    void editor_draw_selection_bounds(const EditorState* editor, const Scene* scene) {
        editor_draw_selection_bounds_internal(editor, scene);
    }

    void editor_draw_ortho_grid(const EditorState* editor, const Viewport& viewport) {
        editor_draw_ortho_grid_internal(editor, viewport);
    }

    bool editor_scene_needs_rebuild(const EditorState* editor) {
        return editor->rebuild_world || editor->rebuild_collision;
    }

    void editor_clear_rebuild_flag(EditorState* editor) {
        editor->rebuild_world = false;
        editor->rebuild_collision = false;
    }

}