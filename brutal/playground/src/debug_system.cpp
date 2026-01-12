#include "debug_system.h"

#include "brutal/core/platform.h"
#include "brutal/core/profiler.h"
#include "brutal/renderer/debug_draw.h"
#include "brutal/renderer/renderer.h"
#include "brutal/world/collision.h"
#include "brutal/world/player.h"
#include <cstdarg>
#include <cstdio>

namespace brutal {

    namespace {

        constexpr const char* kEngineName = "Brutal Engine";
        constexpr const char* kEngineVersion = "0.1.0";

        const char* build_config() {
#if defined(NDEBUG)
            return "Release";
#else
            return "Debug";
#endif
        }

        const char* platform_name() {
#if defined(_WIN32)
            return "Windows";
#elif defined(__APPLE__)
            return "macOS";
#elif defined(__linux__)
            return "Linux";
#else
            return "Unknown";
#endif
        }

        void draw_header(i32& y, const Vec3& color, const char* text) {
            debug_text_printf(10, y, color, "%s", text);
            y += 18;
        }

        void draw_line(i32& y, const Vec3& color, const char* fmt, ...) {
            char buf[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            debug_text_printf(10, y, color, "%s", buf);
            y += 15;
        }

    }

    void debug_system_init(DebugSystem* system) {
        system->show_main = true;
        system->show_perf = false;
        system->show_render = false;
        system->show_collision = false;
        system->show_console = false;
        system->reload_requested = false;
    }

    void debug_system_update(DebugSystem* system, const InputState* input) {
        if (platform_key_pressed(input, KEY_F1)) system->show_main = !system->show_main;
        if (platform_key_pressed(input, KEY_F2)) system->show_perf = !system->show_perf;
        if (platform_key_pressed(input, KEY_F3)) system->show_render = !system->show_render;
        if (platform_key_pressed(input, KEY_F4)) system->show_collision = !system->show_collision;
        if (platform_key_pressed(input, KEY_F5)) system->reload_requested = true;
        if (platform_key_pressed(input, KEY_GRAVE)) system->show_console = !system->show_console;
    }

    bool debug_system_show_collision(const DebugSystem* system) {
        return system->show_collision;
    }

    bool debug_system_consume_reload(DebugSystem* system) {
        bool requested = system->reload_requested;
        system->reload_requested = false;
        return requested;
    }

    void debug_system_draw(const DebugSystem* system,
        const DebugFrameInfo& frame,
        const InputState* input,
        const Player* player,
        const RendererState* renderer,
        const CollisionWorld* collision,
        i32 screen_w,
        i32 screen_h) {
        (void)screen_w;
        (void)screen_h;
        Vec3 white(1, 1, 1);
        Vec3 yellow(1, 1, 0);
        Vec3 green(0, 1, 0);
        Vec3 cyan(0, 1, 1);

        i32 y = 10;

        if (system->show_main) {
            draw_header(y, white, "Brutal Engine - Debug");
            draw_line(y, white, "%s v%s", kEngineName, kEngineVersion);
            draw_line(y, white, "Build: %s  Platform: %s", build_config(), platform_name());
            draw_line(y, white, "FPS: %.1f (%.2f ms)", frame.fps, frame.frame_ms);
            draw_line(y, white, "Delta: %.4f s", frame.delta_time);
            draw_line(y, white, "Player Pos: (%.2f, %.2f, %.2f)",
                player->camera.position.x, player->camera.position.y, player->camera.position.z);
            draw_line(y, white, "Velocity: (%.2f, %.2f, %.2f)",
                player->velocity.x, player->velocity.y, player->velocity.z);
            draw_line(y, white, "Yaw/Pitch: (%.2f, %.2f)", player->camera.yaw, player->camera.pitch);
            draw_line(y, white, "Grounded: %s  Crouched: %s",
                player->grounded ? "true" : "false",
                player->is_crouched ? "true" : "false");
            draw_line(y, white, "WishDir: (%.2f, %.2f, %.2f)",
                player->wish_dir.x, player->wish_dir.y, player->wish_dir.z);
            if (input) {
                bool w = platform_key_down(input, KEY_W);
                bool a = platform_key_down(input, KEY_A);
                bool s = platform_key_down(input, KEY_S);
                bool d = platform_key_down(input, KEY_D);
                bool jump = platform_key_pressed(input, KEY_SPACE);
                bool crouch = platform_key_down(input, KEY_LCONTROL) || platform_key_down(input, KEY_CONTROL);
                draw_line(y, white, "Input W:%d A:%d S:%d D:%d Jump:%d Crouch:%d",
                    w ? 1 : 0, a ? 1 : 0, s ? 1 : 0, d ? 1 : 0,
                    jump ? 1 : 0, crouch ? 1 : 0);
            }
            y += 6;
        }

        if (system->show_perf) {
            draw_header(y, cyan, "Performance HUD");
            draw_line(y, white, "FPS: %.1f (%.2f ms)", frame.fps, frame.frame_ms);
            const FrameProfile* profile = profiler_get_frame();
            if (profile) {
                for (u32 i = 0; i < profile->count; i++) {
                    const ProfileEntry& entry = profile->entries[i];
                    const int indent = (int)entry.depth * 2;
                    debug_text_printf(10 + indent * 6, y, white, "%s: %.3f ms", entry.name, entry.ms);
                    y += 14;
                }
            }
            else {
                draw_line(y, yellow, "Profiler disabled (BRUTAL_ENABLE_PROFILER=0)");
            }
            y += 6;
        }

        if (system->show_render) {
            draw_header(y, green, "Render Stats");
            draw_line(y, white, "Draw Calls: %u", renderer_draw_calls(renderer));
            draw_line(y, white, "Triangles: %u", renderer_triangles(renderer));
            draw_line(y, white, "Vertices: %u", renderer_vertices(renderer));
            if (collision) {
                draw_line(y, white, "Collision Boxes: %u", collision->box_count);
            }
            y += 6;
        }

        if (system->show_collision && collision) {
            Vec3 box_color(1, 0, 0);
            for (u32 i = 0; i < collision->box_count; i++) {
                debug_box(collision->boxes[i], box_color);
            }
            debug_box(player_get_bounds(player), Vec3(0, 1, 0));
        }

        if (system->show_console) {
            draw_header(y, yellow, "` Console (not implemented)");
        }

        debug_text_printf(10, screen_h - 40, yellow,
            "F1 Debug  F2 Perf  F3 Render  F4 Collision  F5 Reload  ` Console");
    }

}