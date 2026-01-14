#include "editor/Editor_gizmo.h"

#include "brutal/math/quat.h"
#include "brutal/world/entity.h"

#include <ImGuizmo.h>
#include <imgui.h>

#include <algorithm>
#include <cstring>

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

    void editor_gizmo_handle_input(EditorContext* ctx, const PlatformState* platform) {
        (void)platform;
        if (!ctx) return;
        if (ctx->gizmo.operation == 0) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::TRANSLATE;
        }
        if (ctx->gizmo.mode == 0) {
            ctx->gizmo.mode = ImGuizmo::MODE::LOCAL;
        }
    }

    void editor_gizmo_draw(EditorContext* ctx, Scene* scene) {
        if (!ctx || !scene) return;
        if (ctx->selection_type == EditorSelectionType::None) return;
        if (!editor_transform_valid(scene, ctx->selection_type, ctx->selection_index)) return;

        if (ctx->viewport.size.x <= 0.0f || ctx->viewport.size.y <= 0.0f) return;

        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ctx->viewport.min.x, ctx->viewport.min.y, ctx->viewport.size.x, ctx->viewport.size.y);

        f32 aspect = ctx->viewport.size.x / ctx->viewport.size.y;
        Mat4 view = camera_view_matrix(&ctx->camera);
        Mat4 proj = camera_projection_matrix(&ctx->camera, aspect);

        Transform transform = {};
        if (!editor_get_transform(scene, ctx->selection_type, ctx->selection_index, &transform)) return;

        Mat4 model = transform_to_matrix(&transform);

        float matrix[16];
        memcpy(matrix, model.m, sizeof(matrix));

        bool snap = false;
        float snap_values[3] = { 1.0f, 1.0f, 1.0f };
        if (ctx->gizmo.operation == ImGuizmo::OPERATION::TRANSLATE && ctx->snap_translate) {
            snap = true;
            snap_values[0] = snap_values[1] = snap_values[2] = ctx->snap_translate_value;
        }
        else if (ctx->gizmo.operation == ImGuizmo::OPERATION::ROTATE && ctx->snap_rotate) {
            snap = true;
            snap_values[0] = snap_values[1] = snap_values[2] = ctx->snap_rotate_value;
        }
        else if (ctx->gizmo.operation == ImGuizmo::OPERATION::SCALE && ctx->snap_scale) {
            snap = true;
            snap_values[0] = snap_values[1] = snap_values[2] = ctx->snap_scale_value;
        }

        ImGuizmo::Manipulate(view.m, proj.m,
            static_cast<ImGuizmo::OPERATION>(ctx->gizmo.operation),
            static_cast<ImGuizmo::MODE>(ctx->gizmo.mode),
            matrix, nullptr, snap ? snap_values : nullptr);

        ctx->gizmo.using_gizmo = ImGuizmo::IsUsing();

        if (ImGuizmo::IsUsing()) {
            float translation[3];
            float rotation[3];
            float scale[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, translation, rotation, scale);
            Transform updated = transform;
            updated.position = Vec3(translation[0], translation[1], translation[2]);
            updated.rotation = quat_from_euler_radians(degrees_to_radians(Vec3(rotation[0], rotation[1], rotation[2])));
            updated.scale = Vec3(scale[0], scale[1], scale[2]);
            editor_set_transform(ctx, scene, ctx->selection_type, ctx->selection_index, updated);
        }
    }

}
