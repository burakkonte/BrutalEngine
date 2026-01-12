#include "brutal/renderer/light.h"

namespace brutal {

void light_environment_init(LightEnvironment* env) {
    env->ambient_color = Vec3(0.1f, 0.1f, 0.15f);
    env->ambient_intensity = 1.0f;
    env->point_light_count = 0;
    for (u32 i = 0; i < MAX_POINT_LIGHTS; i++) env->point_lights[i].active = false;
}

void light_environment_clear(LightEnvironment* env) {
    env->point_light_count = 0;
    for (u32 i = 0; i < MAX_POINT_LIGHTS; i++) env->point_lights[i].active = false;
}

PointLight* light_environment_add_point(LightEnvironment* env, const Vec3& pos, const Vec3& color, f32 radius, f32 intensity) {
    if (env->point_light_count >= MAX_POINT_LIGHTS) return nullptr;
    PointLight* l = &env->point_lights[env->point_light_count++];
    l->position = pos;
    l->color = color;
    l->radius = radius;
    l->intensity = intensity;
    l->active = true;
    return l;
}

}
