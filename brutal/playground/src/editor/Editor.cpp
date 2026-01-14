#include "editor/Editor.h"

#include "editor/Editor_camera.h"
#include "editor/Editor_cursor.h"
#include "editor/Editor_dockspace.h"
#include "editor/Editor_gizmo.h"
#include "editor/Editor_input.h"
#include "editor/Editor_viewport.h"
#include "editor/Panels/Panel_console.h"
#include "editor/Panels/Panel_content.h"
#include "editor/Panels/Panel_hierarchy.h"
#include "editor/Panels/Panel_inspector.h"

#include <ImGuizmo.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace brutal {

    static bool imgui_message_handler(void* hwnd, u32 msg, u64 wparam, i64 lparam) {
        return ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(hwnd), msg, wparam, lparam);
    }

    void editor_init(EditorContext* ctx, PlatformState* platform) {
        if (!ctx || !platform) return;
        *ctx = {};

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(platform->hwnd);
        ImGui_ImplOpenGL3_Init("#version 330");

        platform_set_message_handler(platform, imgui_message_handler);

        camera_init(&ctx->camera);
        ctx->camera.position = Vec3(0.0f, 2.0f, 8.0f);
        ctx->move_speed = 6.0f;
        ctx->look_sensitivity = 0.0025f;

        ctx->show_grid = true;
        ctx->snap_translate = false;
        ctx->snap_translate_value = 0.5f;
        ctx->snap_rotate = false;
        ctx->snap_rotate_value = 15.0f;
        ctx->snap_scale = false;
        ctx->snap_scale_value = 0.1f;

        ctx->gizmo.operation = 0;
        ctx->gizmo.mode = 0;
        ctx->gizmo.using_gizmo = false;
    }

    void editor_shutdown(EditorContext* ctx) {
        if (!ctx) return;
        editor_viewport_destroy(ctx);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void editor_set_active(EditorContext* ctx, bool active, PlatformState* platform, Player* player) {
        if (!ctx || !platform) return;
        ctx->active = active;
        if (active) {
            editor_cursor_set_editor_mode(platform);
        }
        else {
            editor_cursor_set_game_mode(platform);
        }
        (void)player;
    }

    void editor_begin_frame(EditorContext* ctx, PlatformState* platform) {
        if (!ctx || !platform || !ctx->active) return;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void editor_update(EditorContext* ctx, Scene* scene, PlatformState* platform, f32 dt) {
        if (!ctx || !scene || !platform || !ctx->active) return;
        editor_input_update(ctx, platform);
        editor_camera_update(ctx, platform, dt);
        editor_gizmo_handle_input(ctx, platform);
        (void)scene;
    }

    void editor_build_ui(EditorContext* ctx, Scene* scene, PlatformState* platform) {
        if (!ctx || !scene || !platform || !ctx->active) return;

        editor_dockspace_begin(ctx);

        editor_draw_hierarchy(ctx, scene);
        editor_draw_inspector(ctx, scene);
        editor_draw_console(ctx);
        editor_draw_content(ctx, scene);
        editor_draw_viewport(ctx, scene);

        editor_dockspace_end();
    }

    void editor_render_scene(EditorContext* ctx, Scene* scene, RendererState* renderer) {
        if (!ctx || !scene || !renderer || !ctx->active) return;
        editor_viewport_render_scene(ctx, scene, renderer);
    }

    void editor_end_frame(EditorContext* ctx) {
        if (!ctx || !ctx->active) return;
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    bool editor_scene_needs_rebuild(const EditorContext* ctx) {
        return ctx && (ctx->rebuild_world || ctx->rebuild_collision);
    }

    void editor_clear_rebuild_flag(EditorContext* ctx) {
        if (!ctx) return;
        ctx->rebuild_world = false;
        ctx->rebuild_collision = false;
    }

}
