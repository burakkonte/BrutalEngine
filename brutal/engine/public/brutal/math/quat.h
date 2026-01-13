#ifndef BRUTAL_MATH_QUAT_H
#define BRUTAL_MATH_QUAT_H

#include "brutal/math/vec.h"
#include <cmath>

namespace brutal {

    struct Quat {
        f32 x, y, z, w;
    };

    inline Quat quat_identity() {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    inline Quat quat_normalize(const Quat& q) {
        f32 len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len < 1e-6f) return quat_identity();
        f32 inv = 1.0f / len;
        return { q.x * inv, q.y * inv, q.z * inv, q.w * inv };
    }

    inline Quat quat_multiply(const Quat& a, const Quat& b) {
        return {
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        };
    }

    inline Quat quat_from_euler_radians(const Vec3& euler) {
        f32 hx = euler.x * 0.5f;
        f32 hy = euler.y * 0.5f;
        f32 hz = euler.z * 0.5f;
        f32 sx = sinf(hx), cx = cosf(hx);
        f32 sy = sinf(hy), cy = cosf(hy);
        f32 sz = sinf(hz), cz = cosf(hz);

        Quat qx{ sx, 0.0f, 0.0f, cx };
        Quat qy{ 0.0f, sy, 0.0f, cy };
        Quat qz{ 0.0f, 0.0f, sz, cz };
        return quat_normalize(quat_multiply(qz, quat_multiply(qy, qx)));
    }

    inline Vec3 quat_to_euler_radians(const Quat& q_in) {
        Quat q = quat_normalize(q_in);

        f32 sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
        f32 cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        f32 roll = atan2f(sinr_cosp, cosr_cosp);

        f32 sinp = 2.0f * (q.w * q.y - q.z * q.x);
        f32 pitch = 0.0f;
        if (fabsf(sinp) >= 1.0f) {
            pitch = copysignf(3.1415926535f * 0.5f, sinp);
        }
        else {
            pitch = asinf(sinp);
        }

        f32 siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        f32 cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        f32 yaw = atan2f(siny_cosp, cosy_cosp);

        return Vec3(roll, pitch, yaw);
    }

}

#endif
