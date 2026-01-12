#ifndef BRUTAL_MATH_GEOMETRY_H
#define BRUTAL_MATH_GEOMETRY_H

#include "brutal/math/vec.h"

namespace brutal {

struct AABB {
    Vec3 min, max;
};

inline AABB aabb_from_center_size(const Vec3& c, const Vec3& s) {
    Vec3 h = s * 0.5f;
    return {c - h, c + h};
}

inline Vec3 aabb_center(const AABB& b) {
    return (b.min + b.max) * 0.5f;
}

inline Vec3 aabb_half_size(const AABB& b) {
    return (b.max - b.min) * 0.5f;
}

inline bool aabb_intersects(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

f32 aabb_sweep(const AABB& moving, const Vec3& vel, const AABB& stationary, Vec3* normal);

}

#endif
