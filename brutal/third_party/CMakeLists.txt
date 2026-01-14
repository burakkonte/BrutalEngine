cmake_minimum_required(VERSION 3.20)

add_subdirectory(glad)

add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)

add_library(imguizmo STATIC
    ImGuizmo/ImGuizmo.cpp
)
target_include_directories(imguizmo PUBLIC ImGuizmo)