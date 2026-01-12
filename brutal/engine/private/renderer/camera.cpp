#include "brutal/renderer/camera.h"
#include <cmath>

namespace brutal {

// Maximum pitch angle (just under 90 degrees to prevent gimbal lock)
static constexpr f32 MAX_PITCH = 1.553f;  // ~89 degrees in radians

void camera_init(Camera* c) {
    c->position = Vec3(0, 2, 10);
    c->yaw = 0;      // 0 = looking towards -Z
    c->pitch = 0;    // 0 = horizontal, positive = looking up
    c->fov = 1.0472f;  // 60 degrees
    c->near_plane = 0.1f;
    c->far_plane = 1000.0f;
}

void camera_rotate(Camera* c, f32 dyaw, f32 dpitch) {
    c->yaw += dyaw;
    c->pitch += dpitch;
    
    // Clamp pitch to prevent flipping
    if (c->pitch > MAX_PITCH) c->pitch = MAX_PITCH;
    if (c->pitch < -MAX_PITCH) c->pitch = -MAX_PITCH;
    
    // Keep yaw in reasonable range (optional, prevents float precision issues over time)
    const f32 TWO_PI = 6.283185f;
    while (c->yaw > TWO_PI) c->yaw -= TWO_PI;
    while (c->yaw < -TWO_PI) c->yaw += TWO_PI;
}

Vec3 camera_forward(const Camera* c) {
    // Spherical coordinates to cartesian (Y-up, -Z forward at yaw=0)
    f32 cos_pitch = cosf(c->pitch);
    return {
        sinf(c->yaw) * cos_pitch,
        sinf(c->pitch),
        -cosf(c->yaw) * cos_pitch
    };
}

Vec3 camera_right(const Camera* c) {
    // Right vector is always horizontal (Y=0)
    return {cosf(c->yaw), 0, sinf(c->yaw)};
}

Mat4 camera_view_matrix(const Camera* c) {
    Vec3 fwd = camera_forward(c);
    Vec3 target = c->position + fwd;
    return mat4_look_at(c->position, target, Vec3(0, 1, 0));
}

Mat4 camera_projection_matrix(const Camera* c, f32 aspect) {
    return mat4_perspective(c->fov, aspect, c->near_plane, c->far_plane);
}

}
