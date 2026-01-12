#include "brutal/world/entity.h"

namespace brutal {

void entity_init(PropEntity* e, const Vec3& pos, const Vec3& scale, u32 mesh_id, const Vec3& color) {
    e->transform.position = pos;
    e->transform.rotation = Vec3(0, 0, 0);
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
    Mat4 rot_x = mat4_rotation_x(t->rotation.x);
    Mat4 rot_y = mat4_rotation_y(t->rotation.y);
    Mat4 rot_z = mat4_rotation_z(t->rotation.z);
    Mat4 scale = mat4_scale(t->scale);
    Mat4 rot = mat4_multiply(rot_z, mat4_multiply(rot_y, rot_x));
    return mat4_multiply(trans, mat4_multiply(rot, scale));
}

}
