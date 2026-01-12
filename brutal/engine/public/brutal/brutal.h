#ifndef BRUTAL_H
#define BRUTAL_H

#include "brutal/core/types.h"
#include "brutal/core/logging.h"
#include "brutal/core/memory.h"
#include "brutal/core/time.h"
#include "brutal/core/profiler.h"
#include "brutal/core/platform.h"
#include "brutal/math/vec.h"
#include "brutal/math/mat.h"
#include "brutal/math/geometry.h"
#include "brutal/renderer/gl_context.h"
#include "brutal/renderer/shader.h"
#include "brutal/renderer/mesh.h"
#include "brutal/renderer/camera.h"
#include "brutal/renderer/light.h"
#include "brutal/renderer/renderer.h"
#include "brutal/renderer/debug_draw.h"
#include "brutal/world/brush.h"
#include "brutal/world/entity.h"
#include "brutal/world/collision.h"
#include "brutal/world/scene.h"
#include "brutal/world/player.h"

namespace brutal {

struct EngineConfig {
    const char* window_title = "Brutal Engine";
    i32 window_width = 1280;
    i32 window_height = 720;
    size_t persistent_arena_size = 64 * 1024 * 1024;
    size_t frame_arena_size = 16 * 1024 * 1024;
};

struct Engine {
    EngineConfig config;
    PlatformState platform;
    MemoryState memory;
    TimeState time;
    RendererState renderer;
    bool running;
};

bool engine_init(Engine* e, const EngineConfig& cfg);
void engine_shutdown(Engine* e);
void engine_begin_frame(Engine* e);
void engine_end_frame(Engine* e);
bool engine_should_quit(const Engine* e);
MemoryArena* engine_frame_arena(Engine* e);
MemoryArena* engine_persistent_arena(Engine* e);
const FrameTiming* engine_timing(const Engine* e);
InputState* engine_input(Engine* e);
RendererState* engine_renderer(Engine* e);
void engine_set_mouse_capture(Engine* e, bool capture);

}

#endif
