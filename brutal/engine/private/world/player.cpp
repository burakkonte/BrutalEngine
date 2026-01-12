#include "brutal/world/player.h"
#include "brutal/world/collision.h"
#include "brutal/core/platform.h"
#include "brutal/core/profiler.h"
#include <cmath>

namespace brutal {

// =============================================================================
// Physics Constants - tuned for realistic FPS feel
// =============================================================================
static const f32 GRAVITY = 20.0f;              // Gravity acceleration (m/s^2)
static const f32 JUMP_VELOCITY = 6.5f;         // Initial upward velocity when jumping
static const f32 TERMINAL_VELOCITY = 50.0f;    // Max fall speed
static const f32 AIR_CONTROL = 0.3f;           // Air control multiplier (0-1)
static const f32 CROUCH_TRANSITION_SPEED = 8.0f; // Height units per second
static const f32 COYOTE_TIME_MAX = 0.1f;       // Grace period for jumping after leaving ground
static const f32 JUMP_BUFFER_MAX = 0.1f;       // Buffer jump input before landing

// =============================================================================
// Player Initialization
// =============================================================================
void player_init(Player* p) {
    camera_init(&p->camera);
    p->camera.position = Vec3(0, 1.7f, 0);
    p->velocity = Vec3(0, 0, 0);
    
    // Movement speeds (meters per second)
    p->walk_speed = 4.5f;
    p->sprint_speed = 7.5f;
    p->crouch_speed = 2.5f;
    p->sensitivity = 0.002f;
    
    // Physics parameters
    p->gravity = GRAVITY;
    p->jump_velocity = JUMP_VELOCITY;
    p->terminal_velocity = TERMINAL_VELOCITY;
    p->air_control = AIR_CONTROL;
    
    // Physical dimensions
    p->stand_height = 1.8f;
    p->crouch_height = 1.0f;
    p->current_height = p->stand_height;
    p->eye_offset = 0.1f;  // Eyes 10cm below top of head
    p->radius = 0.3f;
    
    // State
    p->move_state = MoveState::STANDING;
    p->grounded = false;
    p->wants_crouch = false;
    p->is_crouched = false;
    p->jump_requested = false;
    p->coyote_time = 0.0f;
    p->jump_buffer_time = 0.0f;
}

// =============================================================================
// Bounds and Position Helpers
// =============================================================================
f32 player_get_feet_y(const Player* p) {
    // Eye is at camera position
    // Eye offset from top = eye_offset
    // So feet = camera.y - (current_height - eye_offset)
    return p->camera.position.y - (p->current_height - p->eye_offset);
}

AABB player_get_bounds(const Player* p) {
    Vec3 eye_pos = p->camera.position;
    f32 feet_y = player_get_feet_y(p);
    Vec3 center(eye_pos.x, feet_y + p->current_height * 0.5f, eye_pos.z);
    Vec3 half(p->radius, p->current_height * 0.5f, p->radius);
    return {center - half, center + half};
}

// Get bounds at a specific height (for collision checking when standing up)
static AABB player_get_bounds_at_height(const Player* p, f32 height) {
    Vec3 eye_pos = p->camera.position;
    f32 feet_y = player_get_feet_y(p);
    Vec3 center(eye_pos.x, feet_y + height * 0.5f, eye_pos.z);
    Vec3 half(p->radius, height * 0.5f, p->radius);
    return {center - half, center + half};
}

bool player_can_stand(const Player* p, const CollisionWorld* col) {
    if (!col || col->box_count == 0) return true;
    
    // Check if standing height would collide with anything
    AABB stand_bounds = player_get_bounds_at_height(p, p->stand_height);
    
    for (u32 i = 0; i < col->box_count; i++) {
        if (aabb_intersects(stand_bounds, col->boxes[i])) {
            return false;
        }
    }
    return true;
}

// =============================================================================
// Main Update
// =============================================================================
void player_update(Player* p, const InputState* input, const CollisionWorld* col, f32 dt) {
    // Clamp dt to prevent physics explosion on lag spikes
    if (dt > 0.1f) dt = 0.1f;
    
    // =========================================================================
    // Mouse Look
    // =========================================================================
    f32 dyaw = (f32)input->mouse.delta_x * p->sensitivity;
    f32 dpitch = (f32)(-input->mouse.delta_y) * p->sensitivity;
    camera_rotate(&p->camera, dyaw, dpitch);
    
    // =========================================================================
    // Jump Input (edge-triggered)
    // =========================================================================
    bool jump_pressed = platform_key_pressed(input, KEY_SPACE);
    if (jump_pressed) {
        p->jump_buffer_time = JUMP_BUFFER_MAX;
    }
    
    // =========================================================================
    // Crouch State (hold to crouch)
    // =========================================================================
    p->wants_crouch = platform_key_down(input, KEY_LCONTROL) || 
                      platform_key_down(input, KEY_CONTROL);
    
    // Determine target height
    f32 old_height = p->current_height;
    f32 target_height;
    if (p->wants_crouch) {
        target_height = p->crouch_height;
        p->is_crouched = true;
    } else {
        // Only stand up if there's room
        if (p->is_crouched && !player_can_stand(p, col)) {
            target_height = p->crouch_height;
            // Stay crouched - can't stand up yet
        } else {
            target_height = p->stand_height;
            p->is_crouched = false;
        }
    }
    
    // Smoothly interpolate height
    f32 height_diff = target_height - p->current_height;
    f32 max_change = CROUCH_TRANSITION_SPEED * dt;
    if (fabsf(height_diff) > max_change) {
        p->current_height += (height_diff > 0) ? max_change : -max_change;
    } else {
        p->current_height = target_height;
    }
    
    // Adjust camera position to maintain feet position during crouch
    // When crouching: feet stay on ground, eye comes down
    if (fabsf(p->current_height - old_height) > 0.0001f) {
        p->camera.position.y += (p->current_height - old_height);
    }
    
    // =========================================================================
    // Movement Input
    // =========================================================================
    f32 fwd = 0, right = 0;
    if (platform_key_down(input, KEY_W)) fwd += 1;
    if (platform_key_down(input, KEY_S)) fwd -= 1;
    if (platform_key_down(input, KEY_A)) right -= 1;
    if (platform_key_down(input, KEY_D)) right += 1;
    
    bool wants_sprint = platform_key_down(input, KEY_SHIFT);
    bool is_moving = (fabsf(fwd) > 0.001f || fabsf(right) > 0.001f);
    
    // =========================================================================
    // Determine Move State
    // =========================================================================
    if (!is_moving) {
        p->move_state = p->is_crouched ? MoveState::CROUCHING : MoveState::STANDING;
    } else if (p->is_crouched) {
        p->move_state = MoveState::CROUCHING;
    } else if (wants_sprint && fwd > 0 && p->grounded) {
        // Can only sprint forward while grounded
        p->move_state = MoveState::SPRINTING;
    } else {
        p->move_state = MoveState::WALKING;
    }
    
    // =========================================================================
    // Calculate Speed
    // =========================================================================
    f32 speed;
    switch (p->move_state) {
        case MoveState::SPRINTING: speed = p->sprint_speed; break;
        case MoveState::CROUCHING: speed = p->crouch_speed; break;
        case MoveState::WALKING:   speed = p->walk_speed; break;
        default:                   speed = 0; break;
    }
    
    // =========================================================================
    // Calculate Movement Direction (horizontal only)
    // =========================================================================
    Vec3 f = camera_forward(&p->camera);
    Vec3 r = camera_right(&p->camera);
    f.y = 0; 
    f = vec3_normalize(f);
    
    Vec3 move_dir = f * fwd + r * right;
    f32 len = vec3_length(move_dir);
    if (len > 0.001f) {
        move_dir = move_dir * (1.0f / len);
    }
    
    // =========================================================================
    // Horizontal Movement (with air control)
    // =========================================================================
    Vec3 target_horizontal = move_dir * speed;
    
    if (p->grounded) {
        // On ground: instant acceleration
        p->velocity.x = target_horizontal.x;
        p->velocity.z = target_horizontal.z;
    } else {
        // In air: limited control
        f32 control = p->air_control;
        p->velocity.x += (target_horizontal.x - p->velocity.x) * control;
        p->velocity.z += (target_horizontal.z - p->velocity.z) * control;
    }
    
    // =========================================================================
    // Jumping (with coyote time and jump buffering)
    // =========================================================================
    // Update coyote time
    if (p->grounded) {
        p->coyote_time = COYOTE_TIME_MAX;
    } else {
        p->coyote_time -= dt;
        if (p->coyote_time < 0) p->coyote_time = 0;
    }
    
    // Update jump buffer
    if (p->jump_buffer_time > 0) {
        p->jump_buffer_time -= dt;
    }
    
    // Execute jump if conditions are met
    bool can_jump = p->coyote_time > 0 && p->velocity.y <= 0.1f;  // Coyote or grounded, not already jumping
    bool want_jump = p->jump_buffer_time > 0;
    
    if (can_jump && want_jump) {
        p->velocity.y = p->jump_velocity;
        p->coyote_time = 0;           // Consume coyote time
        p->jump_buffer_time = 0;      // Consume jump buffer
        p->grounded = false;          // We're airborne now
    }
    
    // =========================================================================
    // Gravity
    // =========================================================================
    if (!p->grounded) {
        p->velocity.y -= p->gravity * dt;
        
        // Clamp to terminal velocity
        if (p->velocity.y < -p->terminal_velocity) {
            p->velocity.y = -p->terminal_velocity;
        }
    }
    
    // =========================================================================
    // Apply Movement with Collision
    // =========================================================================
    Vec3 movement = p->velocity * dt;
    
    if (col && col->box_count > 0) {
        PROFILE_SCOPE("Physics");
        
        AABB bounds = player_get_bounds(p);
        MoveResult result = collision_move_and_slide(col, bounds, movement);
        
        // Calculate actual delta
        Vec3 old_center = aabb_center(bounds);
        Vec3 delta = result.position - old_center;
        p->camera.position = p->camera.position + delta;
        
        // Update grounded state based on collision
        bool grounded_hit = result.hit_floor && p->velocity.y <= 0.0f;
        p->grounded = grounded_hit;
        
        // If we just landed, zero out vertical velocity
        if (grounded_hit && p->velocity.y < 0) {
            p->velocity.y = 0;
        }
        
        // If we hit ceiling, stop upward velocity
        if (result.hit_ceiling && p->velocity.y > 0) {
            p->velocity.y = 0;
        }
    } else {
        // No collision world - just move freely
        p->camera.position = p->camera.position + movement;
        p->grounded = false;
    }
}

}
