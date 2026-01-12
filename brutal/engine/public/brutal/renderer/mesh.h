#ifndef BRUTAL_RENDERER_MESH_H
#define BRUTAL_RENDERER_MESH_H

#include "brutal/core/types.h"
#include "brutal/math/vec.h"

namespace brutal {

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec3 color;
};

struct Mesh {
    u32 vao, vbo, ibo;
    u32 vertex_count;
    u32 index_count;
};

bool mesh_create(Mesh* m, const Vertex* verts, u32 vc, const u32* idx, u32 ic);
void mesh_destroy(Mesh* m);
void mesh_draw(const Mesh* m);
Mesh mesh_create_cube();
Mesh mesh_create_grid(f32 size, i32 divs);

}

#endif
