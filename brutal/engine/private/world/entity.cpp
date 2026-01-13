#include "brutal/world/entity.h"

namespace brutal {

void entity_init(PropEntity* e, const Vec3& pos, const Vec3& scale, u32 mesh_id, const Vec3& color) {
    e->transform.position = pos;
    e->transform.rotation = quat_identity();
    e->transform.scale = scale;
    e->mesh_id = mesh_id;
    e->color = color;
    e->active = true;
}

Mat4 entity_get_transform_matrix(const PropEntity* e) {
    return transform_to_matrix(&e->transform);
}

Mat4 transform_to_matrix(const Transform* t) {
    Mat4 trans = mat4_translation(t->position);
    const Quat r = quat_normalize(t->rotation);
    f32 xx = r.x * r.x;
    f32 yy = r.y * r.y;
    f32 zz = r.z * r.z;
    f32 xy = r.x * r.y;
    f32 xz = r.x * r.z;
    f32 yz = r.y * r.z;
    f32 wx = r.w * r.x;
    f32 wy = r.w * r.y;
    f32 wz = r.w * r.z;
    Mat4 rot = Mat4::identity();
    rot.m[0] = 1.0f - 2.0f * (yy + zz);
    rot.m[1] = 2.0f * (xy + wz);
    rot.m[2] = 2.0f * (xz - wy);
    rot.m[4] = 2.0f * (xy - wz);
    rot.m[5] = 1.0f - 2.0f * (xx + zz);
    rot.m[6] = 2.0f * (yz + wx);
    rot.m[8] = 2.0f * (xz + wy);
    rot.m[9] = 2.0f * (yz - wx);
    rot.m[10] = 1.0f - 2.0f * (xx + yy);
    Mat4 scale = mat4_scale(t->scale);
    return mat4_multiply(trans, mat4_multiply(rot, scale));
}

}
