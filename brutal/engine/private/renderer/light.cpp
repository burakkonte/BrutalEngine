#include "brutal/renderer/light.h"

namespace brutal {

void light_environment_init(LightEnvironment* env) {
    env->ambient_color = Vec3(0.1f, 0.1f, 0.15f);
    env->ambient_intensity = 1.0f;
    env->point_light_count = 0;
    for (u32 i = 0; i < MAX_POINT_LIGHTS; i++) {
        env->point_lights[i].active = false;
        env->point_lights[i].rotation = Vec3(0.0f, 0.0f, 0.0f);
        env->point_lights[i].scale = Vec3(1.0f, 1.0f, 1.0f);
    }
    env->spot_light_count = 0;
    for (u32 i = 0; i < MAX_SPOT_LIGHTS; i++) env->spot_lights[i].active = false;
}

void light_environment_clear(LightEnvironment* env) {
    env->point_light_count = 0;
    for (u32 i = 0; i < MAX_POINT_LIGHTS; i++) {
        env->point_lights[i].active = false;
        env->point_lights[i].rotation = Vec3(0.0f, 0.0f, 0.0f);
        env->point_lights[i].scale = Vec3(1.0f, 1.0f, 1.0f);
    }    env->spot_light_count = 0;
    for (u32 i = 0; i < MAX_SPOT_LIGHTS; i++) env->spot_lights[i].active = false;
}

PointLight* light_environment_add_point(LightEnvironment* env, const Vec3& pos, const Vec3& color, f32 radius, f32 intensity) {
    if (env->point_light_count >= MAX_POINT_LIGHTS) return nullptr;
    PointLight* l = &env->point_lights[env->point_light_count++];
    l->position = pos;
    l->rotation = Vec3(0.0f, 0.0f, 0.0f);
    l->scale = Vec3(1.0f, 1.0f, 1.0f);
    l->color = color;
    l->radius = radius;
    l->intensity = intensity;
    l->active = true;
    return l;
}

SpotLight* light_environment_add_spot(LightEnvironment* env,
    const Vec3& pos,
    const Vec3& direction,
    const Vec3& color,
    f32 range,
    f32 inner_cos,
    f32 outer_cos,
    f32 intensity,
    f32 falloff) {
    if (env->spot_light_count >= MAX_SPOT_LIGHTS) return nullptr;
    SpotLight* l = &env->spot_lights[env->spot_light_count++];
    l->position = pos;
    l->direction = direction;
    l->color = color;
    l->range = range;
    l->inner_cos = inner_cos;
    l->outer_cos = outer_cos;
    l->intensity = intensity;
    l->falloff = falloff;
    l->active = true;
    return l;
}

}
