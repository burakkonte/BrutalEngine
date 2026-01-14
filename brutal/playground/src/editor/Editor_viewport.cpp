#include "editor/Editor_viewport.h"

#include "editor/Editor_gizmo.h"

#include <ImGuizmo.h>
#include <glad/glad.h>
#include <imgui.h>

namespace brutal {

    namespace {

        void editor_framebuffer_create(EditorFramebuffer* fb, i32 width, i32 height) {
            if (!fb) return;
            fb->width = width;
            fb->height = height;

            glGenFramebuffers(1, &fb->fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

            glGenTextures(1, &fb->color_texture);
            glBindTexture(GL_TEXTURE_2D, fb->color_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->color_texture, 0);

            glGenRenderbuffers(1, &fb->depth_rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, fb->depth_rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->depth_rbo);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void editor_framebuffer_destroy(EditorFramebuffer* fb) {
            if (!fb) return;
            if (fb->depth_rbo) {
                glDeleteRenderbuffers(1, &fb->depth_rbo);
            }
            if (fb->color_texture) {
                glDeleteTextures(1, &fb->color_texture);
            }
            if (fb->fbo) {
                glDeleteFramebuffers(1, &fb->fbo);
            }
            *fb = {};
        }

        void editor_framebuffer_resize(EditorFramebuffer* fb, i32 width, i32 height) {
            if (!fb) return;
            if (width <= 0 || height <= 0) return;
            if (fb->width == width && fb->height == height) return;
            editor_framebuffer_destroy(fb);
            editor_framebuffer_create(fb, width, height);
        }

    }

    void editor_draw_viewport(EditorContext* ctx, Scene* scene) {
        if (!ctx || !scene) return;

        ImGui::Begin("Scene");

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Gizmo:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Translate", ctx->gizmo.operation == ImGuizmo::OPERATION::TRANSLATE)) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", ctx->gizmo.operation == ImGuizmo::OPERATION::ROTATE)) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", ctx->gizmo.operation == ImGuizmo::OPERATION::SCALE)) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::SCALE;
        }
        ImGui::SameLine();
        if (ImGui::Button(ctx->gizmo.mode == ImGuizmo::MODE::LOCAL ? "Local" : "World")) {
            ctx->gizmo.mode = (ctx->gizmo.mode == ImGuizmo::MODE::LOCAL)
                ? ImGuizmo::MODE::WORLD
                : ImGuizmo::MODE::LOCAL;
        }

        ImGui::Separator();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x < 1.0f) avail.x = 1.0f;
        if (avail.y < 1.0f) avail.y = 1.0f;

        editor_framebuffer_resize(&ctx->scene_buffer, static_cast<i32>(avail.x), static_cast<i32>(avail.y));

        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(ctx->scene_buffer.color_texture)), avail,
            ImVec2(0, 1), ImVec2(1, 0));

        ImVec2 rect_min = ImGui::GetItemRectMin();
        ImVec2 rect_max = ImGui::GetItemRectMax();
        ctx->viewport.min = Vec2(rect_min.x, rect_min.y);
        ctx->viewport.max = Vec2(rect_max.x, rect_max.y);
        ctx->viewport.size = Vec2(rect_max.x - rect_min.x, rect_max.y - rect_min.y);
        ctx->viewport.hovered = ImGui::IsItemHovered();
        ctx->viewport.focused = ImGui::IsWindowFocused();

        editor_gizmo_draw(ctx, scene);

        ImGui::Text("Viewport: %.1f, %.1f (%.1f x %.1f)",
            ctx->viewport.min.x, ctx->viewport.min.y, ctx->viewport.size.x, ctx->viewport.size.y);
        ImGui::Text("Hovered: %s Focused: %s", ctx->viewport.hovered ? "yes" : "no", ctx->viewport.focused ? "yes" : "no");

        ImGui::End();
    }

    void editor_viewport_render_scene(EditorContext* ctx, Scene* scene, RendererState* renderer) {
        if (!ctx || !scene || !renderer) return;
        if (ctx->scene_buffer.fbo == 0 || ctx->scene_buffer.width <= 0 || ctx->scene_buffer.height <= 0) return;

        glBindFramebuffer(GL_FRAMEBUFFER, ctx->scene_buffer.fbo);
        glViewport(0, 0, ctx->scene_buffer.width, ctx->scene_buffer.height);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        f32 aspect = static_cast<f32>(ctx->scene_buffer.width) / static_cast<f32>(ctx->scene_buffer.height);
        Mat4 view = camera_view_matrix(&ctx->camera);
        Mat4 proj = camera_projection_matrix(&ctx->camera, aspect);
        renderer_set_camera_matrices(renderer, view, proj, ctx->camera.position);

        if (scene->world_mesh.vao) {
            renderer_draw_mesh(renderer, &scene->world_mesh, Mat4::identity(), Vec3(1, 1, 1));
        }

        for (u32 p = 0; p < scene->prop_count; ++p) {
            const PropEntity& prop = scene->props[p];
            if (!prop.active) continue;
            Mat4 model = transform_to_matrix(&prop.transform);
            renderer_draw_mesh(renderer, renderer_get_cube_mesh(renderer), model, prop.color);
        }

        if (!ctx->selection.empty()) {
            const f32 outline_scale = 1.02f;
            for (const auto& item : ctx->selection) {
                if (item.type == EditorSelectionType::Prop) {
                    if (item.index >= scene->prop_count) continue;
                    const PropEntity& prop = scene->props[item.index];
                    if (!prop.active) continue;
                    Mat4 model = transform_to_matrix(&prop.transform);
                    renderer_draw_mesh_outline(renderer, renderer_get_cube_mesh(renderer), model, Vec3(1.0f, 0.85f, 0.2f), outline_scale);
                }
                else if (item.type == EditorSelectionType::Brush) {
                    if (item.index >= scene->brush_count) continue;
                    const Brush& brush = scene->brushes[item.index];
                    AABB brush_aabb = brush_to_aabb(&brush);
                    Vec3 center = aabb_center(brush_aabb);
                    Vec3 size = aabb_half_size(brush_aabb) * 2.0f;
                    Mat4 model = mat4_multiply(mat4_translation(center), mat4_scale(size));
                    renderer_draw_mesh_outline(renderer, renderer_get_cube_mesh(renderer), model, Vec3(1.0f, 0.85f, 0.2f), outline_scale);
                }
                else if (item.type == EditorSelectionType::Light) {
                    if (item.index >= scene->lights.point_light_count) continue;
                    const PointLight& light = scene->lights.point_lights[item.index];
                    Mat4 model = mat4_multiply(mat4_translation(light.position), mat4_scale(Vec3(0.2f, 0.2f, 0.2f)));
                    renderer_draw_mesh_outline(renderer, renderer_get_cube_mesh(renderer), model, Vec3(1.0f, 0.85f, 0.2f), outline_scale);
                }
            }
        }

        if (ctx->show_grid) {
            renderer_draw_grid(renderer);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void editor_viewport_destroy(EditorContext* ctx) {
        if (!ctx) return;
        editor_framebuffer_destroy(&ctx->scene_buffer);
    }

}
