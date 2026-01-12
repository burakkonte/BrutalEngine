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
    private/world/player.cpp
    private/engine.cpp
)

add_library(brutal_engine STATIC ${ENGINE_SOURCES})

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
