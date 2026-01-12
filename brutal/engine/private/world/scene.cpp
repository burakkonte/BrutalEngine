#include "brutal/world/scene.h"
#include "brutal/core/memory.h"
#include "brutal/core/logging.h"

namespace brutal {

bool scene_create(Scene* s, MemoryArena* arena) {
    s->brushes = arena_alloc_array<Brush>(arena, MAX_BRUSHES);
    s->props = arena_alloc_array<PropEntity>(arena, MAX_PROPS);
    if (!s->brushes || !s->props) return false;
    s->brush_count = 0; s->brush_capacity = MAX_BRUSHES;
    s->prop_count = 0; s->prop_capacity = MAX_PROPS;
    s->world_mesh = {}; s->world_mesh_dirty = true;
    light_environment_init(&s->lights);
    collision_world_create(&s->collision, arena, MAX_BRUSHES);
    return true;
}

void scene_destroy(Scene* s) {
    if (s->world_mesh.vao) mesh_destroy(&s->world_mesh);
}

void scene_clear(Scene* s) {
    s->brush_count = 0; s->prop_count = 0;
    s->world_mesh_dirty = true;
    light_environment_clear(&s->lights);
    collision_world_clear(&s->collision);
}

Brush* scene_add_brush(Scene* s, const Vec3& min, const Vec3& max, u32 flags, const Vec3& color) {
    if (s->brush_count >= s->brush_capacity) return nullptr;
    Brush* b = &s->brushes[s->brush_count++];
    b->min = min; b->max = max; b->flags = flags;
    for (int i = 0; i < 6; i++) b->faces[i].color = color;
    s->world_mesh_dirty = true;
    return b;
}

PropEntity* scene_add_prop(Scene* s, const Vec3& pos, const Vec3& scale, u32 mesh_id, const Vec3& color) {
    if (s->prop_count >= s->prop_capacity) return nullptr;
    PropEntity* p = &s->props[s->prop_count++];
    p->transform.position = pos;
    p->transform.rotation = Vec3(0,0,0);
    p->transform.scale = scale;
    p->mesh_id = mesh_id;
    p->color = color;
    p->active = true;
    return p;
}

void scene_rebuild_world_mesh(Scene* s, MemoryArena* temp) {
    if (!s->world_mesh_dirty && s->world_mesh.vao) return;
    u32 vis = 0;
    for (u32 i = 0; i < s->brush_count; i++)
        if (!(s->brushes[i].flags & BRUSH_INVISIBLE)) vis++;
    if (!vis) { s->world_mesh_dirty = false; return; }
    
    Vertex* verts = arena_alloc_array<Vertex>(temp, vis * 24);
    u32* indices = arena_alloc_array<u32>(temp, vis * 36);
    u32 vc = 0, ic = 0;
    for (u32 i = 0; i < s->brush_count; i++) {
        if (s->brushes[i].flags & BRUSH_INVISIBLE) continue;
        vc += brush_generate_vertices(&s->brushes[i], verts + vc);
        brush_generate_indices(vc - 24, indices + ic);
        ic += 36;
    }
    if (s->world_mesh.vao) mesh_destroy(&s->world_mesh);
    mesh_create(&s->world_mesh, verts, vc, indices, ic);
    s->world_mesh_dirty = false;
    LOG_INFO("World mesh: %u verts, %u indices", vc, ic);
}

void scene_rebuild_collision(Scene* s) {
    collision_world_clear(&s->collision);
    for (u32 i = 0; i < s->brush_count; i++) {
        if (s->brushes[i].flags & BRUSH_SOLID)
            collision_world_add_box(&s->collision, brush_to_aabb(&s->brushes[i]));
    }
    LOG_INFO("Collision: %u boxes", s->collision.box_count);
}

}
