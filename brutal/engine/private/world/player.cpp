#include "brutal/world/player.h"
#include "brutal/world/collision.h"
#include "brutal/core/platform.h"
#include "brutal/core/profiler.h"
#include "brutal/core/logging.h"
#include "brutal/world/flashlight.h"
#include <cmath>
#include <algorithm>

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
static const f32 GROUND_ACCEL = 35.0f;         // Ground acceleration (m/s^2)
static const f32 AIR_ACCEL = 12.0f;            // Air acceleration (m/s^2)
static const f32 GROUND_FRICTION = 8.0f;       // Ground friction (1/s)
static const f32 MAX_TIMER_DT = 0.05f;         // Clamp timer dt to avoid spikes
static const f32 JUMP_REQUEST_DUMP_THRESHOLD = 0.2f; // 200ms
static const f32 TWO_PI = 6.283185f;

static f32 clampf(f32 value, f32 min_val, f32 max_val) {
    return std::max(min_val, std::min(value, max_val));
}

static void wrap_yaw(f32& yaw) {
    while (yaw > TWO_PI) yaw -= TWO_PI;
    while (yaw < -TWO_PI) yaw += TWO_PI;
}

static void player_log_jump_frame(Player* p, f32 dt) {
    Player::JumpDebugFrame& frame = p->jump_debug_ring[p->jump_debug_index];
    frame.frame_index = p->jump_debug_frame_index++;
    frame.physics_index = p->jump_debug_physics_index++;
    frame.dt = dt;
    frame.fixed_step_count = p->last_fixed_step_count;
    frame.fixed_step_index = p->fixed_step_index;
    frame.ui_keyboard_capture = p->ui_keyboard_capture;
    frame.space_down = p->jump_down;
    frame.space_pressed_edge = p->jump_pressed_edge;
    frame.space_released_edge = p->jump_released_edge;
    frame.jump_buffer_time = p->jump_buffer_time;
    frame.coyote_time = p->coyote_time;
    frame.grounded = p->grounded;
    frame.grounded_reason = p->grounded_reason;
    frame.vertical_velocity = p->velocity.y;
    frame.jump_requested = p->jump_requested;
    frame.jump_consumed_this_frame = p->jump_consumed_this_frame;
    p->jump_debug_index = (p->jump_debug_index + 1) % Player::kJumpDebugRingSize;
}

static void player_dump_jump_ring(const Player* p, const char* reason) {
    LOG_WARN("==== Jump Debug Dump (%s) ====", reason);
    for (u32 i = 0; i < Player::kJumpDebugRingSize; ++i) {
        u32 idx = (p->jump_debug_index + i) % Player::kJumpDebugRingSize;
        const Player::JumpDebugFrame& f = p->jump_debug_ring[idx];
        LOG_WARN(
            "F%llu P%llu dt=%.4f fixedSteps=%d step=%d ui=%d space(d/p/r)=%d/%d/%d "
            "buf=%.3f coy=%.3f grounded=%d(%s) vy=%.3f req=%d consumed=%d",
            (unsigned long long)f.frame_index,
            (unsigned long long)f.physics_index,
            f.dt,
            f.fixed_step_count,
            f.fixed_step_index,
            f.ui_keyboard_capture ? 1 : 0,
            f.space_down ? 1 : 0,
            f.space_pressed_edge ? 1 : 0,
            f.space_released_edge ? 1 : 0,
            f.jump_buffer_time,
            f.coyote_time,
            f.grounded ? 1 : 0,
            f.grounded_reason ? f.grounded_reason : "unknown",
            f.vertical_velocity,
            f.jump_requested ? 1 : 0,
            f.jump_consumed_this_frame ? 1 : 0);
    }
    LOG_WARN("==== End Jump Debug Dump ====");
}

static void apply_ground_friction(Player* p, f32 dt) {
    f32 speed = sqrtf(p->velocity.x * p->velocity.x + p->velocity.z * p->velocity.z);
    if (speed < 0.0001f) {
        p->velocity.x = 0.0f;
        p->velocity.z = 0.0f;
        return;
    }

    f32 drop = speed * GROUND_FRICTION * dt;
    f32 new_speed = speed - drop;
    if (new_speed < 0.0f) new_speed = 0.0f;
    f32 scale = new_speed / speed;
    p->velocity.x *= scale;
    p->velocity.z *= scale;
}

static void accelerate(Player* p, const Vec3& wish_dir, f32 wish_speed, f32 accel, f32 dt) {
    if (wish_speed <= 0.0f) return;

    f32 current_speed = vec3_dot(p->velocity, wish_dir);
    f32 add_speed = wish_speed - current_speed;
    if (add_speed <= 0.0f) return;

    f32 accel_speed = accel * dt * wish_speed;
    if (accel_speed > add_speed) accel_speed = add_speed;

    p->velocity = p->velocity + wish_dir * accel_speed;
}

// =============================================================================
// Player Initialization
// =============================================================================
void player_init(Player* p) {
    camera_init(&p->camera);
    p->camera.position = Vec3(0, 1.7f, 0);
    p->velocity = Vec3(0, 0, 0);
    p->wish_dir = Vec3(0, 0, 0);
    flashlight_init(&p->flashlight);
    
    // Movement speeds (meters per second)
    p->walk_speed = 4.5f;
    p->sprint_speed = 7.5f;
    p->crouch_speed = 2.5f;
    p->sensitivity = 0.002f;
    p->invert_look_y = false;
    p->enable_look_smoothing = false;
    p->look_smoothing_alpha = 0.25f;
    p->min_pitch = -1.553f;
    p->max_pitch = 1.553f;
    p->look_smoothed = Vec2(0, 0);
    
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
    p->jump_down = false;
    p->jump_pressed_edge = false;
    p->jump_released_edge = false;
    p->ui_keyboard_capture = false;
    p->flashlight_toggle_requested = false;
    p->jump_consumed_this_frame = false;
    p->jump_request_age = 0.0f;
    p->jump_request_dumped = false;
    p->grounded_reason = "init";
    p->last_fixed_dt = 0.0f;
    p->last_frame_dt = 0.0f;
    p->last_fixed_step_count = 0;
    p->fixed_step_index = 0;
    p->jump_debug_index = 0;
    p->jump_debug_frame_index = 0;
    p->jump_debug_physics_index = 0;
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

void player_capture_input(Player* p, const InputState* input, bool ui_keyboard_capture) {
    p->ui_keyboard_capture = ui_keyboard_capture;
    if (!input) {
        p->jump_down = false;
        p->jump_pressed_edge = false;
        p->jump_released_edge = false;
        p->flashlight_toggle_requested = false;
        return;
    }

    p->jump_down = platform_key_down(input, KEY_SPACE);
    p->jump_pressed_edge = platform_key_pressed(input, KEY_SPACE);
    p->jump_released_edge = platform_key_released(input, KEY_SPACE);

    if (!ui_keyboard_capture && p->jump_pressed_edge) {
        p->jump_buffer_time = JUMP_BUFFER_MAX;
        p->jump_requested = true;
        p->jump_request_age = 0.0f;
        p->jump_request_dumped = false;
    }
    if (!ui_keyboard_capture && platform_key_pressed(input, KEY_F)) {
        p->flashlight_toggle_requested = true;
    }
}

void player_set_frame_info(Player* p, f32 frame_dt, i32 fixed_step_count, i32 fixed_step_index) {
    p->last_frame_dt = frame_dt;
    p->last_fixed_step_count = fixed_step_count;
    p->fixed_step_index = fixed_step_index;
}

PlayerLookResult player_apply_mouse_look(Player* p, const InputState* input, bool ui_mouse_capture) {
    PlayerLookResult result = {};
    if (!p || !input) return result;
    if (ui_mouse_capture) {
        p->look_smoothed = Vec2(0, 0);
        return result;
    }

    Vec2 delta(static_cast<f32>(input->mouse.delta_x),
        static_cast<f32>(input->mouse.delta_y));

    if (p->enable_look_smoothing) {
        const f32 alpha = clampf(p->look_smoothing_alpha, 0.0f, 1.0f);
        p->look_smoothed = p->look_smoothed * (1.0f - alpha) + delta * alpha;
        delta = p->look_smoothed;
    }

    const f32 invert = p->invert_look_y ? 1.0f : -1.0f;
    result.yaw_delta = delta.x * p->sensitivity;
    result.pitch_delta = delta.y * p->sensitivity * invert;

    p->camera.yaw += result.yaw_delta;
    p->camera.pitch = clampf(p->camera.pitch + result.pitch_delta, p->min_pitch, p->max_pitch);
    wrap_yaw(p->camera.yaw);

    return result;
}

// =============================================================================
// Main Update
// =============================================================================
void player_update(Player* p, const InputState* input, const CollisionWorld* col, f32 dt) {
    (void)input;
    // Clamp dt to prevent physics explosion on lag spikes
    if (dt > 0.1f) dt = 0.1f;
    p->last_fixed_dt = dt;
    
    
    
    
    
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
    p->wish_dir = move_dir;
    
    // =========================================================================
    // Horizontal Movement (with air control)
    // =========================================================================
    
    if (p->grounded) {
        apply_ground_friction(p, dt);
        accelerate(p, move_dir, speed, GROUND_ACCEL, dt);
    } else {
        // In air: limited control
        f32 air_accel = AIR_ACCEL * p->air_control;
        accelerate(p, move_dir, speed, air_accel, dt);
    }
    
    // =========================================================================
    // Jumping (with coyote time and jump buffering)
    // =========================================================================
    p->jump_consumed_this_frame = false;
    const f32 timer_dt = std::min(dt, MAX_TIMER_DT);
    // Update coyote time
    if (p->grounded) {
        p->coyote_time = COYOTE_TIME_MAX;
    } else {
        p->coyote_time -= timer_dt;
        if (p->coyote_time < 0) p->coyote_time = 0;
    }
    
    // Update jump buffer
    if (p->jump_buffer_time > 0) {
        p->jump_buffer_time -= timer_dt;
        if (p->jump_buffer_time < 0) p->jump_buffer_time = 0.0f;
    }
    
    // Execute jump if conditions are met
    bool can_jump = (p->grounded || p->coyote_time > 0.0f) && !p->jump_consumed_this_frame;    bool want_jump = p->jump_buffer_time > 0;
    
    if (can_jump && want_jump) {
        p->velocity.y = p->jump_velocity;
        p->coyote_time = 0;           // Consume coyote time
        p->jump_buffer_time = 0;      // Consume jump buffer
        p->grounded = false;          // We're airborne now
        p->jump_consumed_this_frame = true;
        p->jump_requested = false;
        p->jump_request_age = 0.0f;
    }

    if (p->jump_requested) {
        p->jump_request_age += dt;
        if (!p->jump_request_dumped && p->jump_request_age >= JUMP_REQUEST_DUMP_THRESHOLD) {
            player_dump_jump_ring(p, "jump request not consumed within 200ms");
            p->jump_request_dumped = true;
        }
        if (p->jump_buffer_time <= 0.0f && !p->jump_consumed_this_frame) {
            p->jump_requested = false;
        }
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
        p->grounded_reason = grounded_hit ? "sweep_hit_floor" : "air";
        
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
        p->grounded_reason = "no_collision";
    }
    player_log_jump_frame(p, dt);
}

void player_update_flashlight(Player* p, f32 dt) {
    if (!p) return;
    if (p->flashlight_toggle_requested) {
        flashlight_toggle(&p->flashlight);
        p->flashlight_toggle_requested = false;
    }
    flashlight_update(&p->flashlight, dt);
}

void player_apply_flashlight(Player* p, const Camera* camera, LightEnvironment* env, bool render_enabled) {
    if (!p) return;
    flashlight_apply_to_renderer(&p->flashlight, camera, env, render_enabled);
}

}
