#ifndef IMGUI_H
#define IMGUI_H

#include <cstdint>

struct ImVec2 {
    float x, y;
    ImVec2(float _x = 0.0f, float _y = 0.0f) : x(_x), y(_y) {}
};

using ImGuiID = unsigned int;
using ImTextureID = void*;
using ImGuiWindowFlags = int;
struct ImGuiViewport {
    ImVec2 Pos;
    ImVec2 Size;
    ImGuiID ID;
};

struct ImGuiIO {
    int ConfigFlags = 0;
    bool WantCaptureMouse = false;
    bool WantCaptureKeyboard = false;
};

enum ImGuiConfigFlags_ {
    ImGuiConfigFlags_None = 0,
    ImGuiConfigFlags_NavEnableKeyboard = 1 << 0,
    ImGuiConfigFlags_DockingEnable = 1 << 6,
    ImGuiConfigFlags_ViewportsEnable = 1 << 10
};

enum ImGuiWindowFlags_ {
    ImGuiWindowFlags_None = 0,
    ImGuiWindowFlags_NoTitleBar = 1 << 0,
    ImGuiWindowFlags_NoResize = 1 << 1,
    ImGuiWindowFlags_NoMove = 1 << 2,
    ImGuiWindowFlags_NoCollapse = 1 << 5,
    ImGuiWindowFlags_NoDocking = 1 << 6,
    ImGuiWindowFlags_NoBringToFrontOnFocus = 1 << 13,
    ImGuiWindowFlags_NoNavFocus = 1 << 19,
    ImGuiWindowFlags_MenuBar = 1 << 20
};

enum ImGuiDockNodeFlags_ {
    ImGuiDockNodeFlags_None = 0,
    ImGuiDockNodeFlags_PassthruCentralNode = 1 << 2,
    ImGuiDockNodeFlags_DockSpace = 1 << 10
};

enum ImGuiDir_ {
    ImGuiDir_None = -1,
    ImGuiDir_Left = 0,
    ImGuiDir_Right = 1,
    ImGuiDir_Up = 2,
    ImGuiDir_Down = 3
};

enum ImGuiTreeNodeFlags_ {
    ImGuiTreeNodeFlags_None = 0,
    ImGuiTreeNodeFlags_DefaultOpen = 1 << 5
};

enum ImGuiStyleVar_ {
    ImGuiStyleVar_WindowRounding = 0,
    ImGuiStyleVar_WindowBorderSize = 1
};

namespace ImGui {

    ImGuiIO& GetIO();
    const ImGuiViewport* GetMainViewport();

    void CreateContext();
    void DestroyContext();
    void StyleColorsDark();
    void NewFrame();
    void Render();
    void* GetDrawData();

    void UpdatePlatformWindows();
    void RenderPlatformWindowsDefault();

    void SetNextWindowPos(const ImVec2& pos);
    void SetNextWindowSize(const ImVec2& size);
    void SetNextWindowViewport(ImGuiID id);
    void PushStyleVar(ImGuiStyleVar_ var, float val);
    void PopStyleVar(int count = 1);

    void Begin(const char* name, bool* p_open = nullptr, int flags = 0);
    void End();

    void DockSpace(ImGuiID id, const ImVec2& size, int flags);
    ImGuiID GetID(const char* str_id);

    void DockBuilderRemoveNode(ImGuiID id);
    void DockBuilderAddNode(ImGuiID id, int flags);
    void DockBuilderSetNodeSize(ImGuiID id, const ImVec2& size);
    ImGuiID DockBuilderSplitNode(ImGuiID id, ImGuiDir_ dir, float ratio, ImGuiID* out_id_at_dir, ImGuiID* out_id_at_opposite_dir);
    void DockBuilderDockWindow(const char* window_name, ImGuiID node_id);
    void DockBuilderFinish(ImGuiID id);

    void AlignTextToFramePadding();
    void TextUnformatted(const char* text);
    void Text(const char* fmt, ...);
    void SameLine();
    void Separator();
    bool RadioButton(const char* label, bool active);
    bool Button(const char* label);
    ImVec2 GetContentRegionAvail();
    void Image(ImTextureID texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1);
    ImVec2 GetItemRectMin();
    ImVec2 GetItemRectMax();
    bool IsItemHovered();
    bool IsWindowFocused();

    bool CollapsingHeader(const char* label, int flags = 0);
    bool Selectable(const char* label, bool selected = false);

    bool DragFloat3(const char* label, float v[3], float v_speed = 1.0f);
    bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f);

    bool Checkbox(const char* label, bool* v);

}

#define IMGUI_CHECKVERSION() ((void)0)

#endif
