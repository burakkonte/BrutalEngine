#ifndef BRUTAL_MATH_MAT_H
#define BRUTAL_MATH_MAT_H

#include "brutal/math/vec.h"
#include <cmath>

namespace brutal {

struct Mat4 {
    f32 m[16];
    
    static Mat4 identity() {
        Mat4 r = {};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
    
    const f32* ptr() const { return m; }
};

inline Mat4 mat4_multiply(const Mat4& a, const Mat4& b) {
    Mat4 r = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                r.m[j*4+i] += a.m[k*4+i] * b.m[j*4+k];
            }
        }
    }
    return r;
}

inline Mat4 mat4_translation(const Vec3& t) {
    Mat4 r = Mat4::identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

inline Mat4 mat4_scale(const Vec3& s) {
    Mat4 r = {};
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
    return r;
}

inline Mat4 mat4_rotation_x(f32 angle) {
    Mat4 r = Mat4::identity();
    f32 c = cosf(angle), s = sinf(angle);
    r.m[5] = c; r.m[6] = s;
    r.m[9] = -s; r.m[10] = c;
    return r;
}

inline Mat4 mat4_rotation_y(f32 angle) {
    Mat4 r = Mat4::identity();
    f32 c = cosf(angle), s = sinf(angle);
    r.m[0] = c; r.m[2] = -s;
    r.m[8] = s; r.m[10] = c;
    return r;
}

inline Mat4 mat4_rotation_z(f32 angle) {
    Mat4 r = Mat4::identity();
    f32 c = cosf(angle), s = sinf(angle);
    r.m[0] = c; r.m[1] = s;
    r.m[4] = -s; r.m[5] = c;
    return r;
}

inline Mat4 mat4_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 f = vec3_normalize(target - eye);
    Vec3 r = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(r, f);
    Mat4 m = Mat4::identity();
    m.m[0] = r.x; m.m[4] = r.y; m.m[8] = r.z;
    m.m[1] = u.x; m.m[5] = u.y; m.m[9] = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -vec3_dot(r, eye);
    m.m[13] = -vec3_dot(u, eye);
    m.m[14] = vec3_dot(f, eye);
    return m;
}

inline Mat4 mat4_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    Mat4 r = {};
    f32 f = 1.0f / tanf(fov * 0.5f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

inline Mat4 mat4_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    Mat4 r = Mat4::identity();
    r.m[0] = 2.0f / (right - left);
    r.m[5] = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (far - near);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(far + near) / (far - near);
    return r;
}

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    return mat4_multiply(a, b);
}

}

#endif
