#ifndef BRUTAL_MATH_VEC_H
#define BRUTAL_MATH_VEC_H

#include "brutal/core/types.h"
#include <cmath>

namespace brutal {

struct Vec2 {
    f32 x, y;
    Vec2() : x(0), y(0) {}
    Vec2(f32 x, f32 y) : x(x), y(y) {}
};

struct Vec3 {
    f32 x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    Vec3 operator*(f32 s) const { return {x*s, y*s, z*s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
};

struct Vec4 {
    f32 x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, f32 w) : x(v.x), y(v.y), z(v.z), w(w) {}
};

inline f32 vec3_dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline Vec3 vec3_cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

inline f32 vec3_length(const Vec3& v) {
    return sqrtf(vec3_dot(v, v));
}

inline Vec3 vec3_normalize(const Vec3& v) {
    f32 len = vec3_length(v);
    return len > 0.0001f ? v * (1.0f / len) : Vec3(0, 0, 0);
}

}

#endif
