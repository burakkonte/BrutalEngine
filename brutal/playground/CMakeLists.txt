cmake_minimum_required(VERSION 3.20)

add_executable(playground
    src/main.cpp
    src/debug_system.cpp
    src/editor/Editor.cpp
    src/editor/Editor_camera.cpp
    src/editor/Editor_cursor.cpp
    src/editor/Editor_dockspace.cpp
    src/editor/Editor_gizmo.cpp
    src/editor/Editor_input.cpp
    src/editor/Editor_viewport.cpp
    src/editor/Panels/Panel_console.cpp
    src/editor/Panels/Panel_content.cpp
    src/editor/Panels/Panel_hierarchy.cpp
    src/editor/Panels/Panel_inspector.cpp
)
target_link_libraries(playground PRIVATE brutal_engine imgui imguizmo)
target_include_directories(playground PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR})

if(WIN32)
    # Link Windows subsystem for console app
    target_link_options(playground PRIVATE /SUBSYSTEM:CONSOLE)
endif()
