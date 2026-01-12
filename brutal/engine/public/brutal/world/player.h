#ifndef BRUTAL_WORLD_PLAYER_H
#define BRUTAL_WORLD_PLAYER_H

#include "brutal/renderer/camera.h"
#include "brutal/math/geometry.h"

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
    
    // Movement parameters
    f32 walk_speed;
    f32 sprint_speed;
    f32 crouch_speed;
    f32 sensitivity;
    
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
};

void player_init(Player* p);
void player_update(Player* p, const InputState* input, const CollisionWorld* col, f32 dt);
AABB player_get_bounds(const Player* p);

// Check if player can stand up (no ceiling collision)
bool player_can_stand(const Player* p, const CollisionWorld* col);

// Get feet Y position
f32 player_get_feet_y(const Player* p);

}

#endif
