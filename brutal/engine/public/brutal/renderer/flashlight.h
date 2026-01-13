#ifndef BRUTAL_WORLD_FLASHLIGHT_H
#define BRUTAL_WORLD_FLASHLIGHT_H

#include "brutal/core/types.h"
#include "brutal/math/vec.h"
#include "brutal/renderer/camera.h"
#include "brutal/renderer/light.h"

namespace brutal {

    struct FlashlightConfig {
        f32 range;
        f32 intensity;
        f32 inner_angle_deg;
        f32 outer_angle_deg;
        Vec3 color;
        f32 falloff;
        Vec3 position_offset;
        bool enable_flicker;
        f32 flicker_strength;
        f32 flicker_speed;
        f32 battery_drain_per_sec;
    };

    struct PlayerFlashlight {
        FlashlightConfig config;
        bool enabled;
        f32 battery_level;
        f32 flicker_phase;
        f32 intensity_scale;
        u32 spot_light_index;
        bool spot_light_registered;
    };

    void flashlight_init(PlayerFlashlight* flashlight);
    void flashlight_enable(PlayerFlashlight* flashlight);
    void flashlight_disable(PlayerFlashlight* flashlight);
    void flashlight_toggle(PlayerFlashlight* flashlight);
    void flashlight_update(PlayerFlashlight* flashlight, f32 dt);
    void flashlight_apply_to_renderer(PlayerFlashlight* flashlight, const Camera* camera, LightEnvironment* env, bool render_enabled);

}

#endif
