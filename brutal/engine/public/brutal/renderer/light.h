#ifndef BRUTAL_RENDERER_LIGHT_H
#define BRUTAL_RENDERER_LIGHT_H

#include "brutal/math/vec.h"
#include "brutal/core/types.h"

namespace brutal {

constexpr u32 MAX_POINT_LIGHTS = 16;
constexpr u32 MAX_SPOT_LIGHTS = 8;

struct PointLight {
    Vec3 position;
    f32 radius;
    Vec3 color;
    f32 intensity;
    bool active;
};

struct SpotLight {
    Vec3 position;
    f32 range;
    Vec3 direction;
    f32 inner_cos;
    Vec3 color;
    f32 intensity;
    f32 outer_cos;
    f32 falloff;
    bool active;
};

struct LightEnvironment {
    Vec3 ambient_color;
    f32 ambient_intensity;
    PointLight point_lights[MAX_POINT_LIGHTS];
    u32 point_light_count;
    SpotLight spot_lights[MAX_SPOT_LIGHTS];
    u32 spot_light_count;
};

void light_environment_init(LightEnvironment* env);
void light_environment_clear(LightEnvironment* env);
PointLight* light_environment_add_point(LightEnvironment* env, const Vec3& pos, const Vec3& color, f32 radius, f32 intensity);
SpotLight* light_environment_add_spot(LightEnvironment* env,
    const Vec3& pos,
    const Vec3& direction,
    const Vec3& color,
    f32 range,
    f32 inner_cos,
    f32 outer_cos,
    f32 intensity,
    f32 falloff);
}

#endif
