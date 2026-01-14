#include "editor/Panels/Panel_hierarchy.h"

#include <imgui.h>

#include <cstdio>

namespace brutal {

    namespace {

        void editor_set_selection(EditorContext* ctx, EditorSelectionType type, u32 index) {
            if (!ctx) return;
            ctx->selection_type = type;
            ctx->selection_index = index;
            ctx->selection.clear();
            if (type != EditorSelectionType::None) {
                ctx->selection.push_back({ type, index });
            }
        }

    }

    void editor_draw_hierarchy(EditorContext* ctx, Scene* scene) {
        if (!ctx || !scene) return;
        ImGui::Begin("Hierarchy");

        if (ImGui::CollapsingHeader("Brushes", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (u32 i = 0; i < scene->brush_count; ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Brush %u", i);
                bool selected = ctx->selection_type == EditorSelectionType::Brush && ctx->selection_index == i;
                if (ImGui::Selectable(label, selected)) {
                    editor_set_selection(ctx, EditorSelectionType::Brush, i);
                }
            }
        }

        if (ImGui::CollapsingHeader("Props", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (u32 i = 0; i < scene->prop_count; ++i) {
                const PropEntity& prop = scene->props[i];
                if (!prop.active) continue;
                char label[64];
                snprintf(label, sizeof(label), "Prop %u", i);
                bool selected = ctx->selection_type == EditorSelectionType::Prop && ctx->selection_index == i;
                if (ImGui::Selectable(label, selected)) {
                    editor_set_selection(ctx, EditorSelectionType::Prop, i);
                }
            }
        }

        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (u32 i = 0; i < scene->lights.point_light_count; ++i) {
                const PointLight& light = scene->lights.point_lights[i];
                if (!light.active) continue;
                char label[64];
                snprintf(label, sizeof(label), "Light %u", i);
                bool selected = ctx->selection_type == EditorSelectionType::Light && ctx->selection_index == i;
                if (ImGui::Selectable(label, selected)) {
                    editor_set_selection(ctx, EditorSelectionType::Light, i);
                }
            }
        }

        if (ImGui::Button("Clear Selection")) {
            editor_set_selection(ctx, EditorSelectionType::None, 0);
        }

        ImGui::End();
    }

}
