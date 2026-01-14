#include "editor/Panels/Panel_inspector.h"

#include "brutal/math/quat.h"

#include <imgui.h>

#include <algorithm>

namespace brutal {

    namespace {

        constexpr f32 kDegreesToRadians = 0.0174532925f;
        constexpr f32 kRadiansToDegrees = 57.295779513f;

        bool editor_transform_valid(const Scene* scene, EditorSelectionType type, u32 index) {
            switch (type) {
            case EditorSelectionType::Brush:
                return index < scene->brush_count;
            case EditorSelectionType::Prop:
                return index < scene->prop_count;
            case EditorSelectionType::Light:
                return index < scene->lights.point_light_count;
            default:
                return false;
            }
        }

        bool editor_get_transform(const Scene* scene, EditorSelectionType type, u32 index, Transform* out) {
            if (!editor_transform_valid(scene, type, index)) return false;
            if (type == EditorSelectionType::Prop) {
                if (out) *out = scene->props[index].transform;
                return true;
            }
            if (type == EditorSelectionType::Brush) {
                const Brush& brush = scene->brushes[index];
                AABB bounds = brush_to_aabb(&brush);
                if (out) {
                    out->position = aabb_center(bounds);
                    out->rotation = quat_identity();
                    out->scale = aabb_half_size(bounds) * 2.0f;
                }
                return true;
            }
            if (type == EditorSelectionType::Light) {
                const PointLight& light = scene->lights.point_lights[index];
                if (out) {
                    out->position = light.position;
                    out->rotation = quat_from_euler_radians(light.rotation);
                    out->scale = light.scale;
                }
                return true;
            }
            return false;
        }

        void editor_set_transform(EditorContext* ctx, Scene* scene, EditorSelectionType type, u32 index, const Transform& transform) {
            if (!ctx || !editor_transform_valid(scene, type, index)) return;
            if (type == EditorSelectionType::Prop) {
                PropEntity& prop = scene->props[index];
                prop.transform = transform;
                prop.transform.scale.x = std::max(prop.transform.scale.x, 0.05f);
                prop.transform.scale.y = std::max(prop.transform.scale.y, 0.05f);
                prop.transform.scale.z = std::max(prop.transform.scale.z, 0.05f);
                return;
            }
            if (type == EditorSelectionType::Brush) {
                Brush& brush = scene->brushes[index];
                Vec3 size = transform.scale;
                size.x = std::max(size.x, 0.1f);
                size.y = std::max(size.y, 0.1f);
                size.z = std::max(size.z, 0.1f);
                Vec3 half = size * 0.5f;
                brush.min = transform.position - half;
                brush.max = transform.position + half;
                ctx->rebuild_world = true;
                ctx->rebuild_collision = true;
                return;
            }
            if (type == EditorSelectionType::Light) {
                PointLight& light = scene->lights.point_lights[index];
                light.position = transform.position;
                light.rotation = quat_to_euler_radians(transform.rotation);
                light.scale = transform.scale;
                return;
            }
        }

        Vec3 radians_to_degrees(const Vec3& v) {
            return Vec3(v.x * kRadiansToDegrees, v.y * kRadiansToDegrees, v.z * kRadiansToDegrees);
        }

        Vec3 degrees_to_radians(const Vec3& v) {
            return Vec3(v.x * kDegreesToRadians, v.y * kDegreesToRadians, v.z * kDegreesToRadians);
        }

    }

    void editor_draw_inspector(EditorContext* ctx, Scene* scene) {
        if (!ctx || !scene) return;
        ImGui::Begin("Inspector");

        if (ctx->selection_type == EditorSelectionType::None) {
            ImGui::TextUnformatted("No selection.");
            ImGui::End();
            return;
        }

        Transform transform;
        if (!editor_get_transform(scene, ctx->selection_type, ctx->selection_index, &transform)) {
            ImGui::TextUnformatted("Selection invalid.");
            ImGui::End();
            return;
        }

        Vec3 rotation_deg = radians_to_degrees(quat_to_euler_radians(transform.rotation));

        bool changed = false;
        changed |= ImGui::DragFloat3("Position", &transform.position.x, 0.05f);
        changed |= ImGui::DragFloat3("Rotation", &rotation_deg.x, 0.5f);
        changed |= ImGui::DragFloat3("Scale", &transform.scale.x, 0.05f);

        if (changed) {
            transform.rotation = quat_from_euler_radians(degrees_to_radians(rotation_deg));
            editor_set_transform(ctx, scene, ctx->selection_type, ctx->selection_index, transform);
        }

        ImGui::Separator();
        ImGui::Checkbox("Snap Translate", &ctx->snap_translate);
        ImGui::SameLine();
        ImGui::DragFloat("##snap_translate_value", &ctx->snap_translate_value, 0.1f, 0.01f, 100.0f);
        ImGui::Checkbox("Snap Rotate", &ctx->snap_rotate);
        ImGui::SameLine();
        ImGui::DragFloat("##snap_rotate_value", &ctx->snap_rotate_value, 1.0f, 1.0f, 90.0f);
        ImGui::Checkbox("Snap Scale", &ctx->snap_scale);
        ImGui::SameLine();
        ImGui::DragFloat("##snap_scale_value", &ctx->snap_scale_value, 0.01f, 0.01f, 10.0f);

        ImGui::Checkbox("Show Grid", &ctx->show_grid);

        ImGui::End();
    }

}
