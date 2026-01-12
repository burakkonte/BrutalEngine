#ifndef BRUTAL_RENDERER_LIGHT_H
#define BRUTAL_RENDERER_LIGHT_H

#include "brutal/math/vec.h"
#include "brutal/core/types.h"

namespace brutal {

constexpr u32 MAX_POINT_LIGHTS = 16;

struct PointLight {
    Vec3 position;
    f32 radius;
    Vec3 color;
    f32 intensity;
    bool active;
};

struct LightEnvironment {
    Vec3 ambient_color;
    f32 ambient_intensity;
    PointLight point_lights[MAX_POINT_LIGHTS];
    u32 point_light_count;
};

void light_environment_init(LightEnvironment* env);
void light_environment_clear(LightEnvironment* env);
PointLight* light_environment_add_point(LightEnvironment* env, const Vec3& pos, const Vec3& color, f32 radius, f32 intensity);

}

#endif
