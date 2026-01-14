#ifndef IMGUI_IMPL_OPENGL3_H
#define IMGUI_IMPL_OPENGL3_H

bool ImGui_ImplOpenGL3_Init(const char* glsl_version);
bool ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(void* draw_data);
void ImGui_ImplOpenGL3_Shutdown();

#endif
