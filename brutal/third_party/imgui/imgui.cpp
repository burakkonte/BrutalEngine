#include "imgui.h"

#include <cstdarg>
#include <cstdio>

namespace {

    ImGuiIO g_io;
    ImGuiViewport g_viewport{ ImVec2(0.0f, 0.0f), ImVec2(1280.0f, 720.0f), 1 };
    ImVec2 g_last_min(0.0f, 0.0f);
    ImVec2 g_last_max(0.0f, 0.0f);

}

namespace ImGui {

    ImGuiIO& GetIO() { return g_io; }

    const ImGuiViewport* GetMainViewport() { return &g_viewport; }

    void CreateContext() {}
    void DestroyContext() {}
    void StyleColorsDark() {}
    void NewFrame() {}
    void Render() {}
    void* GetDrawData() { return nullptr; }

    void UpdatePlatformWindows() {}
    void RenderPlatformWindowsDefault() {}

    void SetNextWindowPos(const ImVec2& pos) { g_viewport.Pos = pos; }
    void SetNextWindowSize(const ImVec2& size) { g_viewport.Size = size; }
    void SetNextWindowViewport(ImGuiID id) { g_viewport.ID = id; }
    void PushStyleVar(ImGuiStyleVar_, float) {}
    void PopStyleVar(int) {}

    void Begin(const char*, bool*, int) {}
    void End() {}

    void DockSpace(ImGuiID, const ImVec2&, int) {}
    ImGuiID GetID(const char*) { return 1; }

    void DockBuilderRemoveNode(ImGuiID) {}
    void DockBuilderAddNode(ImGuiID, int) {}
    void DockBuilderSetNodeSize(ImGuiID, const ImVec2&) {}
    ImGuiID DockBuilderSplitNode(ImGuiID, ImGuiDir_, float, ImGuiID*, ImGuiID*) { return 1; }
    void DockBuilderDockWindow(const char*, ImGuiID) {}
    void DockBuilderFinish(ImGuiID) {}

    void AlignTextToFramePadding() {}
    void TextUnformatted(const char*) {}
    void Text(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
    }
    void SameLine() {}
    void Separator() {}
    bool RadioButton(const char*, bool) { return false; }
    bool Button(const char*) { return false; }
    ImVec2 GetContentRegionAvail() { return g_viewport.Size; }
    void Image(ImTextureID, const ImVec2& size, const ImVec2&, const ImVec2&) {
        g_last_min = ImVec2(0.0f, 0.0f);
        g_last_max = ImVec2(size.x, size.y);
    }
    ImVec2 GetItemRectMin() { return g_last_min; }
    ImVec2 GetItemRectMax() { return g_last_max; }
    bool IsItemHovered() { return false; }
    bool IsWindowFocused() { return false; }

    bool CollapsingHeader(const char*, int) { return true; }
    bool Selectable(const char*, bool) { return false; }

    bool DragFloat3(const char*, float[3], float) { return false; }
    bool DragFloat(const char*, float*, float, float, float) { return false; }

    bool Checkbox(const char*, bool*) { return false; }

}
