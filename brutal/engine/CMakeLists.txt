set(ENGINE_SOURCES
    private/core/logging.cpp
    private/core/memory.cpp
    private/core/profiler.cpp
    private/core/time.cpp
    private/core/platform_win32.cpp
    private/math/geometry.cpp
    private/renderer/gl_context.cpp
    private/renderer/shader.cpp
    private/renderer/mesh.cpp
    private/renderer/renderer.cpp
    private/renderer/camera.cpp
    private/renderer/light.cpp
    private/renderer/debug_draw.cpp
    private/world/brush.cpp
    private/world/entity.cpp
    private/world/collision.cpp
    private/world/scene.cpp
    private/world/scene_io.cpp
    private/world/player.cpp
    private/engine.cpp
)

set(FLASHLIGHT_SRC private/world/flashlight.cpp)
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${FLASHLIGHT_SRC}")
    list(APPEND ENGINE_SOURCES ${FLASHLIGHT_SRC})
endif()

if (NOT TARGET brutal_engine)
    add_library(brutal_engine STATIC ${ENGINE_SOURCES} "public/brutal/world/flashlight.h" "public/brutal/math/quat.h" "public/brutal/world/scene_io.h")
endif()

target_include_directories(brutal_engine
    PUBLIC public
    PRIVATE private
)

target_compile_definitions(brutal_engine
    PUBLIC
        $<$<CONFIG:Release>:BRUTAL_ENABLE_PROFILER=0>
        $<$<NOT:$<CONFIG:Release>>:BRUTAL_ENABLE_PROFILER=1>
)

target_link_libraries(brutal_engine
    PUBLIC glad
    PRIVATE user32 gdi32 opengl32 winmm
)
