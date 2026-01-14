#include "imgui_impl_win32.h"

bool ImGui_ImplWin32_Init(void*) { return true; }
bool ImGui_ImplWin32_NewFrame() { return true; }
void ImGui_ImplWin32_Shutdown() {}
bool ImGui_ImplWin32_WndProcHandler(void*, unsigned int, std::uint64_t, std::int64_t) { return false; }
