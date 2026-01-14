#include "editor/Editor_input.h"

#include <ImGuizmo.h>
#include <imgui.h>

namespace brutal {

    void editor_input_update(EditorContext* ctx, const PlatformState* platform) {
        if (!ctx || !platform) return;
        ImGuiIO& io = ImGui::GetIO();
        ctx->wants_capture_mouse = io.WantCaptureMouse;
        ctx->wants_capture_keyboard = io.WantCaptureKeyboard;

        if (ctx->wants_capture_keyboard) return;

        if (platform_key_pressed(&platform->input, KEY_W)) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::TRANSLATE;
        }
        if (platform_key_pressed(&platform->input, KEY_E)) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::ROTATE;
        }
        if (platform_key_pressed(&platform->input, KEY_R)) {
            ctx->gizmo.operation = ImGuizmo::OPERATION::SCALE;
        }
        if (platform_key_pressed(&platform->input, KEY_Q)) {
            ctx->gizmo.mode = (ctx->gizmo.mode == ImGuizmo::MODE::LOCAL)
                ? ImGuizmo::MODE::WORLD
                : ImGuizmo::MODE::LOCAL;
        }
    }

}
