#ifndef BRUTAL_RENDERER_CAMERA_H
#define BRUTAL_RENDERER_CAMERA_H

#include "brutal/math/vec.h"
#include "brutal/math/mat.h"

namespace brutal {

struct Camera {
    Vec3 position;
    f32 yaw, pitch;
    f32 fov, near_plane, far_plane;
};

void camera_init(Camera* c);
void camera_rotate(Camera* c, f32 dyaw, f32 dpitch);
Vec3 camera_forward(const Camera* c);
Vec3 camera_right(const Camera* c);
Mat4 camera_view_matrix(const Camera* c);
Mat4 camera_projection_matrix(const Camera* c, f32 aspect);

}

#endif
