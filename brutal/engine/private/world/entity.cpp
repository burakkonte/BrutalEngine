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
    Mat4 trans = mat4_translation(e->transform.position);
    Mat4 scale = mat4_scale(e->transform.scale);
    return mat4_multiply(trans, scale);
}

}
