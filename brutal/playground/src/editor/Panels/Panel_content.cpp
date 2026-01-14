#include "editor/Panels/Panel_content.h"

#include <imgui.h>

namespace brutal {

    void editor_draw_content(EditorContext* ctx, Scene* scene) {
        if (!ctx || !scene) return;
        ImGui::Begin("Content Browser");
        ImGui::TextUnformatted("Assets placeholder.");
        ImGui::Text("Brushes: %u", scene->brush_count);
        ImGui::Text("Props: %u", scene->prop_count);
        ImGui::Text("Lights: %u", scene->lights.point_light_count);
        ImGui::End();
    }

}
