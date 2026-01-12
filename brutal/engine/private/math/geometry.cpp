#include "brutal/math/geometry.h"
#include <cmath>

namespace brutal {

f32 aabb_sweep(const AABB& moving, const Vec3& vel, const AABB& stationary, Vec3* normal) {
    if (normal) *normal = Vec3(0, 0, 0);
    
    Vec3 half = aabb_half_size(moving);
    AABB exp = {stationary.min - half, stationary.max + half};
    Vec3 o = aabb_center(moving);
    
    f32 t_enter = 0.0f, t_exit = 1.0f;
    Vec3 enter_normal(0, 0, 0);
    
    f32 axes[3] = {vel.x, vel.y, vel.z};
    f32 origins[3] = {o.x, o.y, o.z};
    f32 bmins[3] = {exp.min.x, exp.min.y, exp.min.z};
    f32 bmaxs[3] = {exp.max.x, exp.max.y, exp.max.z};
    
    for (int i = 0; i < 3; i++) {
        if (fabsf(axes[i]) < 0.00001f) {
            if (origins[i] < bmins[i] || origins[i] > bmaxs[i]) {
                return 1.0f;
            }
        } else {
            f32 t1 = (bmins[i] - origins[i]) / axes[i];
            f32 t2 = (bmaxs[i] - origins[i]) / axes[i];
            Vec3 n1(0, 0, 0), n2(0, 0, 0);
            if (i == 0) { n1.x = -1; n2.x = 1; }
            else if (i == 1) { n1.y = -1; n2.y = 1; }
            else { n1.z = -1; n2.z = 1; }
            
            if (t1 > t2) {
                f32 tmp = t1; t1 = t2; t2 = tmp;
                Vec3 tmpn = n1; n1 = n2; n2 = tmpn;
            }
            
            if (t1 > t_enter) {
                t_enter = t1;
                enter_normal = n1;
            }
            if (t2 < t_exit) {
                t_exit = t2;
            }
            
            if (t_enter > t_exit) return 1.0f;
            if (t_exit < 0.0f) return 1.0f;
        }
    }
    
    if (t_enter < 0.0f) {
        return 1.0f;
    }
    
    if (t_enter > 1.0f) {
        return 1.0f;
    }
    
    if (normal) *normal = enter_normal;
    return t_enter;
}

}
