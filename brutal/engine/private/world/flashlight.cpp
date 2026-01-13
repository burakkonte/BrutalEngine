#include "brutal/world/flashlight.h"
#include "brutal/renderer/camera.h"
#include "brutal/renderer/light.h"
#include "brutal/math/vec.h"
#include <algorithm>
#include <cmath>

namespace brutal {

    static f32 clampf(f32 v, f32 min_v, f32 max_v) {
        return std::max(min_v, std::min(v, max_v));
    }

    static f32 deg_to_rad(f32 deg) {
        return deg * 0.0174532925f;
    }

    void flashlight_init(PlayerFlashlight* flashlight) {
        flashlight->config.range = 18.0f;
        flashlight->config.intensity = 6.0f;
        flashlight->config.inner_angle_deg = 12.0f;
        flashlight->config.outer_angle_deg = 20.0f;
        flashlight->config.color = Vec3(0.95f, 0.98f, 1.0f);
        flashlight->config.falloff = 2.0f;
        flashlight->config.position_offset = Vec3(0.05f, -0.03f, 0.15f);
        flashlight->config.enable_flicker = false;
        flashlight->config.flicker_strength = 0.03f;
        flashlight->config.flicker_speed = 12.0f;
        flashlight->config.battery_drain_per_sec = 0.0f;
        flashlight->enabled = false;
        flashlight->battery_level = 1.0f;
        flashlight->flicker_phase = 0.0f;
        flashlight->intensity_scale = 1.0f;
        flashlight->spot_light_index = 0;
        flashlight->spot_light_registered = false;
    }

    void flashlight_enable(PlayerFlashlight* flashlight) {
        if (!flashlight) return;
        if (flashlight->battery_level <= 0.0f) return;
        flashlight->enabled = true;
        // audio_play_event("flashlight_click_on");
    }

    void flashlight_disable(PlayerFlashlight* flashlight) {
        if (!flashlight) return;
        flashlight->enabled = false;
        // audio_play_event("flashlight_click_off");
    }

    void flashlight_toggle(PlayerFlashlight* flashlight) {
        if (!flashlight) return;
        if (flashlight->enabled) {
            flashlight_disable(flashlight);
        }
        else {
            flashlight_enable(flashlight);
        }
    }

    void flashlight_update(PlayerFlashlight* flashlight, f32 dt) {
        if (!flashlight) return;

        if (flashlight->enabled && flashlight->config.battery_drain_per_sec > 0.0f) {
            flashlight->battery_level = clampf(
                flashlight->battery_level - flashlight->config.battery_drain_per_sec * dt, 0.0f, 1.0f);
            if (flashlight->battery_level <= 0.0f) {
                flashlight->enabled = false;
            }
        }

        flashlight->intensity_scale = 1.0f;
        if (flashlight->enabled && flashlight->config.enable_flicker) {
            flashlight->flicker_phase += flashlight->config.flicker_speed * dt;
            f32 flicker = sinf(flashlight->flicker_phase) * flashlight->config.flicker_strength;
            flashlight->intensity_scale = clampf(1.0f + flicker, 0.0f, 1.5f);
        }
    }

    void flashlight_apply_to_renderer(PlayerFlashlight* flashlight, const Camera* camera, LightEnvironment* env, bool render_enabled) {
        if (!flashlight || !camera || !env) return;

        Vec3 forward = vec3_normalize(camera_forward(camera));
        Vec3 right = vec3_normalize(camera_right(camera));
        Vec3 up = vec3_normalize(vec3_cross(right, forward));

        Vec3 pos = camera->position
            + right * flashlight->config.position_offset.x
            + up * flashlight->config.position_offset.y
            + forward * flashlight->config.position_offset.z;

        f32 inner_rad = deg_to_rad(flashlight->config.inner_angle_deg);
        f32 outer_rad = deg_to_rad(flashlight->config.outer_angle_deg);
        f32 inner_cos = cosf(inner_rad);
        f32 outer_cos = cosf(outer_rad);

        f32 intensity = flashlight->config.intensity * flashlight->intensity_scale;
        if (!flashlight->enabled || !render_enabled) {
            intensity = 0.0f;
        }

        if (!flashlight->spot_light_registered) {
            SpotLight* spot = light_environment_add_spot(
                env,
                pos,
                forward,
                flashlight->config.color,
                flashlight->config.range,
                inner_cos,
                outer_cos,
                intensity,
                flashlight->config.falloff);
            if (spot) {
                flashlight->spot_light_index = env->spot_light_count - 1;
                flashlight->spot_light_registered = true;
            }
            return;
        }

        if (flashlight->spot_light_index >= env->spot_light_count) return;
        SpotLight& spot = env->spot_lights[flashlight->spot_light_index];
        spot.position = pos;
        spot.direction = forward;
        spot.color = flashlight->config.color;
        spot.range = flashlight->config.range;
        spot.inner_cos = inner_cos;
        spot.outer_cos = outer_cos;
        spot.intensity = intensity;
        spot.falloff = flashlight->config.falloff;
        spot.active = intensity > 0.0f;
    }

}
