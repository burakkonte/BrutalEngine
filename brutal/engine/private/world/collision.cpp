#include "brutal/world/collision.h"
#include "brutal/core/memory.h"
#include "brutal/core/logging.h"
#include <cmath>

namespace brutal {

bool collision_world_create(CollisionWorld* w, MemoryArena* arena, u32 cap) {
    w->boxes = arena_alloc_array<AABB>(arena, cap);
    if (!w->boxes) return false;
    w->box_count = 0;
    w->box_capacity = cap;
    return true;
}

void collision_world_clear(CollisionWorld* w) { w->box_count = 0; }

void collision_world_add_box(CollisionWorld* w, const AABB& box) {
    if (w->box_count < w->box_capacity) w->boxes[w->box_count++] = box;
}

// Resolve penetration if player is already overlapping a box
// Returns the push-out vector
static Vec3 resolve_penetration(const Vec3& pos, const Vec3& half, const AABB& box) {
    Vec3 bc = aabb_center(box);
    Vec3 bh = aabb_half_size(box);
    
    // Calculate overlap on each axis
    f32 dx = (half.x + bh.x) - fabsf(pos.x - bc.x);
    f32 dy = (half.y + bh.y) - fabsf(pos.y - bc.y);
    f32 dz = (half.z + bh.z) - fabsf(pos.z - bc.z);
    
    // If not actually penetrating on all axes, no push needed
    if (dx <= 0 || dy <= 0 || dz <= 0) return Vec3(0, 0, 0);
    
    // Push out along the axis of minimum penetration
    Vec3 push(0, 0, 0);
    if (dx <= dy && dx <= dz) {
        push.x = (pos.x > bc.x) ? dx : -dx;
    } else if (dy <= dz) {
        push.y = (pos.y > bc.y) ? dy : -dy;
    } else {
        push.z = (pos.z > bc.z) ? dz : -dz;
    }
    return push;
}

MoveResult collision_move_and_slide(const CollisionWorld* w, const AABB& player, const Vec3& vel) {
    const f32 SKIN = 0.005f;      // Separation distance from walls
    const int MAX_ITER = 5;       // Maximum slide iterations
    const f32 MIN_MOVE = 0.0001f; // Minimum movement threshold
    
    MoveResult r = {};
    Vec3 pos = aabb_center(player);
    Vec3 half = aabb_half_size(player);
    Vec3 rem = vel;
    
    // Phase 1: Resolve any existing penetrations
    // This handles cases where the player somehow got inside geometry
    for (int resolve_iter = 0; resolve_iter < 4; resolve_iter++) {
        Vec3 total_push(0, 0, 0);
        bool any_penetration = false;
        
        for (u32 i = 0; i < w->box_count; i++) {
            Vec3 push = resolve_penetration(pos, half, w->boxes[i]);
            if (fabsf(push.x) > MIN_MOVE || fabsf(push.y) > MIN_MOVE || fabsf(push.z) > MIN_MOVE) {
                // Add a small skin distance to the push
                if (push.x > 0) push.x += SKIN; else if (push.x < 0) push.x -= SKIN;
                if (push.y > 0) push.y += SKIN; else if (push.y < 0) push.y -= SKIN;
                if (push.z > 0) push.z += SKIN; else if (push.z < 0) push.z -= SKIN;
                
                total_push = total_push + push;
                any_penetration = true;
                
                // Track what we hit
                if (fabsf(push.y) > fabsf(push.x) && fabsf(push.y) > fabsf(push.z)) {
                    if (push.y > 0) r.hit_floor = true;
                    else r.hit_ceiling = true;
                } else {
                    r.hit_wall = true;
                }
            }
        }
        
        if (!any_penetration) break;
        pos = pos + total_push;
    }
    
    // Phase 2: Move and slide with collision response
    for (int iter = 0; iter < MAX_ITER; iter++) {
        f32 rem_len_sq = rem.x * rem.x + rem.y * rem.y + rem.z * rem.z;
        if (rem_len_sq < MIN_MOVE * MIN_MOVE) break;
        
        AABB moving = {pos - half, pos + half};
        f32 closest_t = 1.0f;
        Vec3 closest_n(0, 0, 0);
        
        for (u32 i = 0; i < w->box_count; i++) {
            Vec3 n;
            f32 t = aabb_sweep(moving, rem, w->boxes[i], &n);
            if (t < closest_t) {
                closest_t = t;
                closest_n = n;
            }
        }
        
        if (closest_t < 1.0f) {
            // We hit something
            r.hit_normal = closest_n;
            if (closest_n.y > 0.5f) r.hit_floor = true;
            else if (closest_n.y < -0.5f) r.hit_ceiling = true;
            else r.hit_wall = true;
            
            // Move to just before the collision point
            f32 safe_t = closest_t - (SKIN / sqrtf(rem_len_sq));
            if (safe_t < 0.0f) safe_t = 0.0f;
            pos = pos + rem * safe_t;
            
            // Calculate remaining movement
            rem = rem * (1.0f - closest_t);
            
            // Project remaining velocity onto the collision plane (slide)
            f32 dot = vec3_dot(rem, closest_n);
            rem = rem - closest_n * dot;
            
            // Remove tiny velocity components to prevent jitter
            if (fabsf(rem.x) < MIN_MOVE) rem.x = 0;
            if (fabsf(rem.y) < MIN_MOVE) rem.y = 0;
            if (fabsf(rem.z) < MIN_MOVE) rem.z = 0;
        } else {
            // No collision, apply full remaining movement
            pos = pos + rem;
            break;
        }
    }
    
    // Phase 3: Final penetration check after movement
    // This catches edge cases where sliding puts us into another wall
    for (int resolve_iter = 0; resolve_iter < 2; resolve_iter++) {
        bool any_penetration = false;
        for (u32 i = 0; i < w->box_count; i++) {
            Vec3 push = resolve_penetration(pos, half, w->boxes[i]);
            if (fabsf(push.x) > MIN_MOVE || fabsf(push.y) > MIN_MOVE || fabsf(push.z) > MIN_MOVE) {
                if (push.x > 0) push.x += SKIN; else if (push.x < 0) push.x -= SKIN;
                if (push.y > 0) push.y += SKIN; else if (push.y < 0) push.y -= SKIN;
                if (push.z > 0) push.z += SKIN; else if (push.z < 0) push.z -= SKIN;
                pos = pos + push;
                any_penetration = true;
            }
        }
        if (!any_penetration) break;
    }
    
    r.position = pos;
    return r;
}

}
