#ifndef BRUTAL_WORLD_COLLISION_H
#define BRUTAL_WORLD_COLLISION_H

#include "brutal/math/geometry.h"

namespace brutal {

struct MemoryArena;

struct CollisionWorld {
    AABB* boxes;
    u32 box_count, box_capacity;
};

bool collision_world_create(CollisionWorld* w, MemoryArena* arena, u32 cap);
void collision_world_clear(CollisionWorld* w);
void collision_world_add_box(CollisionWorld* w, const AABB& box);

struct MoveResult {
    Vec3 position;
    bool hit_wall, hit_floor, hit_ceiling;
    Vec3 hit_normal;
};

MoveResult collision_move_and_slide(const CollisionWorld* w, const AABB& box, const Vec3& vel);

}

#endif
