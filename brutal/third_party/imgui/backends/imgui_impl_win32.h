#ifndef IMGUI_IMPL_WIN32_H
#define IMGUI_IMPL_WIN32_H

#include <cstdint>

struct ImGuiViewport;

bool ImGui_ImplWin32_Init(void* hwnd);
bool ImGui_ImplWin32_NewFrame();
void ImGui_ImplWin32_Shutdown();
bool ImGui_ImplWin32_WndProcHandler(void* hwnd, unsigned int msg, std::uint64_t wparam, std::int64_t lparam);

#endif
