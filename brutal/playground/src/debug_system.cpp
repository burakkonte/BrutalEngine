#include "debug_system.h"

#include "brutal/core/platform.h"
#include "brutal/core/profiler.h"
#include "brutal/renderer/debug_draw.h"
#include "brutal/renderer/renderer.h"
#include "brutal/world/collision.h"
#include "brutal/world/player.h"
#include "brutal/world/scene.h"
#include <cmath>
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

        void draw_point_light_gizmo(const PointLight& light) {
            Vec3 color = light.color;
            const f32 r = light.radius;
            const Vec3 p = light.position;
            debug_line(p + Vec3(-r, 0, 0), p + Vec3(r, 0, 0), color);
            debug_line(p + Vec3(0, -r, 0), p + Vec3(0, r, 0), color);
            debug_line(p + Vec3(0, 0, -r), p + Vec3(0, 0, r), color);
            debug_wire_box(p, Vec3(r * 2.0f, r * 2.0f, r * 2.0f), color);
        }

        void draw_spot_light_gizmo(const SpotLight& light) {
            Vec3 color = light.color;
            Vec3 dir = vec3_normalize(light.direction);
            Vec3 up(0.0f, 1.0f, 0.0f);
            if (fabsf(vec3_dot(dir, up)) > 0.9f) {
                up = Vec3(1.0f, 0.0f, 0.0f);
            }
            Vec3 right = vec3_normalize(vec3_cross(dir, up));
            Vec3 up_axis = vec3_normalize(vec3_cross(right, dir));

            f32 angle = acosf(light.outer_cos);
            f32 radius = tanf(angle) * light.range;
            Vec3 base_center = light.position + dir * light.range;

            const int segments = 8;
            Vec3 prev = base_center + right * radius;
            for (int i = 1; i <= segments; ++i) {
                f32 t = (f32)i / (f32)segments;
                f32 theta = t * 6.283185f;
                Vec3 offset = right * cosf(theta) * radius + up_axis * sinf(theta) * radius;
                Vec3 point = base_center + offset;
                debug_line(prev, point, color);
                prev = point;
            }

            debug_line(light.position, base_center + right * radius, color);
            debug_line(light.position, base_center - right * radius, color);
            debug_line(light.position, base_center + up_axis * radius, color);
            debug_line(light.position, base_center - up_axis * radius, color);
        }

    }

    void debug_system_init(DebugSystem* system) {
        system->show_main = true;
        system->show_perf = false;
        system->show_render = false;
        system->show_collision = false;
        system->show_lights = false;
        system->show_player_bounds = false;
        system->show_console = false;
        system->reload_requested = false;
    }

    void debug_system_update(DebugSystem* system, const InputState* input) {
        if (platform_key_pressed(input, KEY_F1)) system->show_main = !system->show_main;
        if (platform_key_pressed(input, KEY_F2)) system->show_perf = !system->show_perf;
        if (platform_key_pressed(input, KEY_F3)) system->show_render = !system->show_render;
        if (platform_key_pressed(input, KEY_F4)) system->show_collision = !system->show_collision;
        if (platform_key_pressed(input, KEY_F5)) system->show_lights = !system->show_lights;
        if (platform_key_pressed(input, KEY_F6)) system->show_player_bounds = !system->show_player_bounds;
        if (platform_key_pressed(input, KEY_F7)) system->reload_requested = true;
        if (platform_key_pressed(input, KEY_GRAVE)) system->show_console = !system->show_console;
    }

    bool debug_system_show_collision(const DebugSystem* system) {
        return system->show_collision;
    }

    bool debug_system_has_world_lines(const DebugSystem* system) {
        if (!system) return false;
        return system->show_collision || system->show_lights || system->show_player_bounds;
    }

    bool debug_system_consume_reload(DebugSystem* system) {
        bool requested = system->reload_requested;
        system->reload_requested = false;
        return requested;
    }

    void debug_system_draw(const DebugSystem* system,
        const DebugFrameInfo& frame,
        const InputState* input,
        const PlatformState* platform,
        const Player* player,
        const RendererState* renderer,
        const Scene* scene,
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
            draw_line(y, white, "Jump Down:%d Pressed:%d Released:%d UI:%d",
                player->jump_down ? 1 : 0,
                player->jump_pressed_edge ? 1 : 0,
                player->jump_released_edge ? 1 : 0,
                player->ui_keyboard_capture ? 1 : 0);
            draw_line(y, white, "Jump Buffer: %.3f  Coyote: %.3f  Requested:%d Consumed:%d",
                player->jump_buffer_time,
                player->coyote_time,
                player->jump_requested ? 1 : 0,
                player->jump_consumed_this_frame ? 1 : 0);
            draw_line(y, white, "Fixed dt: %.4f  FixedSteps:%d StepIdx:%d",
                player->last_fixed_dt,
                player->last_fixed_step_count,
                player->fixed_step_index);
            draw_line(y, white, "WishDir: (%.2f, %.2f, %.2f)",
                player->wish_dir.x, player->wish_dir.y, player->wish_dir.z);
            if (input) {
                const bool w = platform_key_down(input, KEY_W);
                const bool a = platform_key_down(input, KEY_A);
                const bool s = platform_key_down(input, KEY_S);
                const bool d = platform_key_down(input, KEY_D);
                const bool jump = platform_key_pressed(input, KEY_SPACE);
                const bool crouch = platform_key_down(input, KEY_LCONTROL) || platform_key_down(input, KEY_RCONTROL);
                draw_line(y, white, "Input W:%d A:%d S:%d D:%d Jump:%d Crouch:%d",
                    w ? 1 : 0, a ? 1 : 0, s ? 1 : 0, d ? 1 : 0,
                    jump ? 1 : 0, crouch ? 1 : 0);
            }
            y += 6;
        }

        if (platform) {
            const MouseLookTelemetryFrame* look = platform_mouse_look_latest(platform);
            if (look) {
                draw_line(y, yellow,
                    "Mouse raw(%d,%d) consumed(%d,%d) dt=%.3fms stutter(dt=%d dx=%d) look=%d ui=%d",
                    look->raw_dx, look->raw_dy,
                    look->consumed_dx, look->consumed_dy,
                    look->frame_ms,
                    look->dt_spike ? 1 : 0,
                    look->dx_spike ? 1 : 0,
                    look->mouse_look_enabled ? 1 : 0,
                    look->ui_mouse_capture ? 1 : 0);
            }
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

        }

        if (system->show_player_bounds && player) {
            debug_box(player_get_bounds(player), Vec3(0, 1, 0));
        }

        if (system->show_lights && scene) {
            for (u32 i = 0; i < scene->lights.point_light_count; ++i) {
                const PointLight& light = scene->lights.point_lights[i];
                if (!light.active) continue;
                draw_point_light_gizmo(light);
            }
            for (u32 i = 0; i < scene->lights.spot_light_count; ++i) {
                const SpotLight& light = scene->lights.spot_lights[i];
                if (!light.active) continue;
                draw_spot_light_gizmo(light);
            }
        }

        if (system->show_console) {
            draw_header(y, yellow, "` Console (not implemented)");
        }

        debug_text_printf(10, screen_h - 40, yellow,
            "F1 Debug  F2 Perf  F3 Render  F4 Collision  F5 Lights  F6 Bounds  F7 Reload  ` Console");
    }

}