#ifndef PLAYGROUND_DEBUG_SYSTEM_H
#define PLAYGROUND_DEBUG_SYSTEM_H

#include "brutal/core/types.h"

namespace brutal {

    struct CollisionWorld;
    struct InputState;
    struct PlatformState;
    struct Player;
    struct RendererState;

    struct DebugFrameInfo {
        f32 delta_time;
        f32 frame_ms;
        f32 fps;
    };

    struct DebugSystem {
        bool show_main;
        bool show_perf;
        bool show_render;
        bool show_collision;
        bool show_console;
        bool reload_requested;
    };

    void debug_system_init(DebugSystem* system);
    void debug_system_update(DebugSystem* system, const InputState* input);
    void debug_system_draw(const DebugSystem* system,
        const DebugFrameInfo& frame,
        const InputState* input,
        const PlatformState* platform,
        const Player* player,
        const RendererState* renderer,
        const CollisionWorld* collision,
        i32 screen_w,
        i32 screen_h);
    bool debug_system_show_collision(const DebugSystem* system);
    bool debug_system_consume_reload(DebugSystem* system);

}

#endif