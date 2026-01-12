#ifndef BRUTAL_WORLD_SCENE_H
#define BRUTAL_WORLD_SCENE_H

#include "brutal/world/brush.h"
#include "brutal/world/entity.h"
#include "brutal/world/collision.h"
#include "brutal/renderer/light.h"
#include "brutal/renderer/mesh.h"

namespace brutal {

struct MemoryArena;

constexpr u32 MAX_BRUSHES = 256;
constexpr u32 MAX_PROPS = 128;

struct Scene {
    Brush* brushes;
    u32 brush_count, brush_capacity;
    Mesh world_mesh;
    bool world_mesh_dirty;
    PropEntity* props;
    u32 prop_count, prop_capacity;
    LightEnvironment lights;
    CollisionWorld collision;
};

bool scene_create(Scene* s, MemoryArena* arena);
void scene_destroy(Scene* s);
void scene_clear(Scene* s);
Brush* scene_add_brush(Scene* s, const Vec3& min, const Vec3& max, u32 flags, const Vec3& color);
PropEntity* scene_add_prop(Scene* s, const Vec3& pos, const Vec3& scale, u32 mesh_id, const Vec3& color);
void scene_rebuild_world_mesh(Scene* s, MemoryArena* temp);
void scene_rebuild_collision(Scene* s);

}

#endif
