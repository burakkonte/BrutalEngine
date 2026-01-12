#ifndef BRUTAL_WORLD_ENTITY_H
#define BRUTAL_WORLD_ENTITY_H

#include "brutal/math/vec.h"
#include "brutal/math/mat.h"

namespace brutal {

struct Transform {
    Vec3 position, rotation, scale;
};

inline Transform transform_default() {
    return {Vec3(0,0,0), Vec3(0,0,0), Vec3(1,1,1)};
}

Mat4 transform_to_matrix(const Transform* t);

struct PropEntity {
    Transform transform;
    u32 mesh_id;
    Vec3 color;
    bool active;
};

constexpr u32 MESH_CUBE = 0;

}

#endif
