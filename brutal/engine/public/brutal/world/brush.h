#ifndef BRUTAL_WORLD_BRUSH_H
#define BRUTAL_WORLD_BRUSH_H

#include "brutal/math/vec.h"
#include "brutal/math/geometry.h"

namespace brutal {

struct Vertex;

constexpr u32 BRUSH_SOLID = 1;
constexpr u32 BRUSH_INVISIBLE = 2;

struct BrushFace { Vec3 color; };

struct Brush {
    Vec3 min, max;
    BrushFace faces[6];
    u32 flags;
};

inline AABB brush_to_aabb(const Brush* b) { return {b->min, b->max}; }
u32 brush_generate_vertices(const Brush* b, Vertex* out);
u32 brush_generate_indices(u32 base, u32* out);

}

#endif
