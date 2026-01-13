#ifndef BRUTAL_WORLD_PLAYER_H
#define BRUTAL_WORLD_PLAYER_H

#include "brutal/renderer/camera.h"
#include "brutal/math/geometry.h"
#include "brutal/math/vec.h"
#include "brutal/core/types.h"
#include "brutal/world/flashlight.h"

namespace brutal {

struct InputState;
struct CollisionWorld;

// Movement state for deterministic behavior
enum class MoveState {
    STANDING,
    WALKING,
    SPRINTING,
    CROUCHING
};

struct Player {
    Camera camera;
    Vec3 velocity;
    Vec3 wish_dir;
    PlayerFlashlight flashlight;
    
    // Movement parameters
    f32 walk_speed;
    f32 sprint_speed;
    f32 crouch_speed;
    f32 sensitivity;
    bool invert_look_y;
    bool enable_look_smoothing;
    f32 look_smoothing_alpha;
    f32 min_pitch;
    f32 max_pitch;
    Vec2 look_smoothed;
    
    // Physics parameters (NEW - for proper gravity/jump)
    f32 gravity;           // Gravity acceleration (m/s^2)
    f32 jump_velocity;     // Initial upward velocity when jumping
    f32 terminal_velocity; // Max fall speed
    f32 air_control;       // Air control multiplier (0-1)
    
    // Physical dimensions
    f32 stand_height;      // Full standing height (1.8m)
    f32 crouch_height;     // Crouched height (1.0m)
    f32 current_height;    // Interpolated current height
    f32 eye_offset;        // Eye position from top of player (0.1m below top)
    f32 radius;
    
    // State
    MoveState move_state;
    bool grounded;
    bool wants_crouch;     // Player holding crouch key
    bool is_crouched;      // Actually crouched (may differ if can't stand up)
    
    // Jump state (NEW - for edge-triggered jump with coyote time)
    bool jump_requested;   // Jump was requested this frame
    f32 coyote_time;       // Time since last grounded (for late jumps)
    f32 jump_buffer_time;  // Time since jump was pressed (for early jumps)

    // Input edges (captured per render frame)
    bool jump_down;
    bool jump_pressed_edge;
    bool jump_released_edge;
    bool ui_keyboard_capture;
    bool flashlight_toggle_requested;

    // Jump debug telemetry
    bool jump_consumed_this_frame;
    f32 jump_request_age;
    bool jump_request_dumped;
    const char* grounded_reason;
    f32 last_fixed_dt;
    f32 last_frame_dt;
    i32 last_fixed_step_count;
    i32 fixed_step_index;

    // Jump debug ring buffer (last 120 frames)
    static constexpr u32 kJumpDebugRingSize = 120;
    struct JumpDebugFrame {
        u64 frame_index;
        u64 physics_index;
        f32 dt;
        i32 fixed_step_count;
        i32 fixed_step_index;
        bool ui_keyboard_capture;
        bool space_down;
        bool space_pressed_edge;
        bool space_released_edge;
        f32 jump_buffer_time;
        f32 coyote_time;
        bool grounded;
        const char* grounded_reason;
        f32 vertical_velocity;
        bool jump_requested;
        bool jump_consumed_this_frame;
    } jump_debug_ring[kJumpDebugRingSize];
    u32 jump_debug_index;
    u64 jump_debug_frame_index;
    u64 jump_debug_physics_index;
};

void player_init(Player* p);
void player_capture_input(Player* p, const InputState* input, bool ui_keyboard_capture);
void player_set_frame_info(Player* p, f32 frame_dt, i32 fixed_step_count, i32 fixed_step_index);
struct PlayerLookResult {
    f32 yaw_delta;
    f32 pitch_delta;
};
PlayerLookResult player_apply_mouse_look(Player* p, const InputState* input, bool ui_mouse_capture);
void player_update(Player* p, const InputState* input, const CollisionWorld* col, f32 dt);
void player_update_flashlight(Player* p, f32 dt);
void player_apply_flashlight(Player* p, const Camera* camera, LightEnvironment* env, bool render_enabled);
AABB player_get_bounds(const Player* p);

// Check if player can stand up (no ceiling collision)
bool player_can_stand(const Player* p, const CollisionWorld* col);

// Get feet Y position
f32 player_get_feet_y(const Player* p);

}

#endif
