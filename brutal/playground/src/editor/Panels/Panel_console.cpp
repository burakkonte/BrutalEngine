#include "editor/Panels/Panel_console.h"

#include <imgui.h>

namespace brutal {

    void editor_draw_console(EditorContext* ctx) {
        if (!ctx) return;
        ImGui::Begin("Console");
        ImGui::TextUnformatted("Console output will appear here.");
        ImGui::Text("Input Capture: mouse=%s keyboard=%s", ctx->wants_capture_mouse ? "yes" : "no",
            ctx->wants_capture_keyboard ? "yes" : "no");
        ImGui::End();
    }

}
