# Brutal Engine FPS Player Controller Design

This document outlines a pragmatic, high-reliability FPS player controller layout and jump system for a single gothic FPS horror game. It focuses on correctness, determinism, and debuggability over general-purpose abstractions.

## 1) Recommended Player file layout

> One `Player` type, multiple feature-focused compilation units.

**Public API**
- `engine/public/brutal/world/Player.h`
  - Public `Player` struct/class and minimal public methods.
  - Exposes a `Player::UpdateFixed()` for physics, `Player::UpdateRender()` for visuals, and accessors.

**Private implementation**
- `engine/private/world/Player.cpp`
  - Construction / initialization, default constants, and shared helpers.
- `engine/private/world/Player_Input.cpp`
  - Input sampling, UI/editor capture handling, input buffering, and command generation.
- `engine/private/world/Player_Movement.cpp`
  - Ground/air movement, acceleration, friction, slopes, step-up, sweep/slide integration.
- `engine/private/world/Player_Jump.cpp`
  - Jump buffer, coyote time, consume rules, and jump state transitions.
- `engine/private/world/Player_Camera.cpp`
  - View angles, head bob, FOV kick, camera crouch offsets.
- `engine/private/world/Player_Audio.cpp`
  - Footsteps, landing impacts, jump/land events.
- `engine/private/world/Player_Interact.cpp`
  - Use/interact traces, pickup logic, prompts.
- `engine/private/world/Player_Debug.cpp`
  - Debug logging, ring buffer dump, overlay hooks.

> **Single-game focus:** Keep one data model and fewer layers. Avoid ECS-level decoupling for this scope.

## 2) Robust input-to-movement pipeline

### Goals
- Never miss a jump press, even with fixed-step physics or UI focus transitions.
- Deterministic consume ordering (no double-jumps from the same press).
- Minimal APIs: input -> commands -> movement.

### Pipeline
1. **Input Sampling (per-frame, render/update loop)**
   - Gather raw input with focus awareness.
   - Generate _edges_ (`pressed`, `released`) and _holds_.
   - Store input into a **command buffer** with timestamps.

2. **Command Buffering (frame-rate domain)**
   - On `JumpPressed`, set `jump_buffer_time = kJumpBuffer`.
   - Update `moveAxis`, `lookDelta`, `sprintHeld`, `crouchHeld` every frame.

3. **Fixed-Step Consumption (physics update loop)**
   - For each fixed step, decrement timers using fixed `dt`.
   - Consume `jump_buffer_time` if conditions are valid (grounded/coyote).
   - Apply movement in a consistent order.

> **Key invariant:** Input edges are captured in the render/update thread, then consumed in fixed-step updates. This prevents missed SPACE presses when `UpdateFixed()` runs less frequently than `UpdateRender()`.

## 3) Jump system design (buffer + coyote)

### State & timers
- `jump_buffer_time` (seconds): set on press, decays in fixed update.
- `coyote_time` (seconds): set when grounded, decays in fixed update.
- `jump_consumed` (bool): ensures the same press is consumed once.

### Deterministic consume rules
- **Consume when**:
  - `jump_buffer_time > 0`, and
  - (`grounded == true` **OR** `coyote_time > 0`), and
  - `jump_consumed == false`.
- **On consume**:
  - Set vertical velocity to jump impulse.
  - Clear `jump_buffer_time`.
  - Set `jump_consumed = true`.
  - Clear `coyote_time`.
- **Reset consume**:
  - When grounded again **and** jump released OR after a short grace if needed.

### Update order (fixed-step pseudocode)
1. Decrement `jump_buffer_time` / `coyote_time`.
2. Refresh grounded state (collision/sweep results).
3. If grounded: `coyote_time = kCoyoteTime`.
4. Evaluate jump consume rules.
5. Apply movement: acceleration, friction, air control.
6. Apply gravity (after jump).
7. Move with sweep/slide, resolve steps/slopes.

> **Avoid race conditions:** All timers are updated in the same fixed-step domain. The input layer only sets buffer time when the key edge arrives.

## 4) Debugging plan for intermittent SPACE issues

### Log/overlay per frame
- `space_down`, `space_pressed`, `space_released`
- `jump_buffer_time`
- `coyote_time`
- `grounded` and `grounded_normal`
- `jump_consumed`
- `fixed_dt`, `frame_dt`, `fixed_step_count`
- `input_focused` / `ui_captured`

### Common causes and fast verification
1. **Missed edge due to fixed-step mismatch**
   - Symptom: `space_pressed` happens between physics ticks.
   - Verify: log render frames vs fixed ticks; ensure buffer time set in render thread.

2. **Input captured by UI/editor**
   - Symptom: `space_down` true in OS but false in engine.
   - Verify: log `ui_captured`, ensure input sampling gates on capture.

3. **Incorrect timer reset order**
   - Symptom: buffer cleared before consume.
   - Verify: trace timer values at each stage; ensure consume occurs before buffer clear.

4. **Grounding instability**
   - Symptom: `grounded` flickers; coyote never set.
   - Verify: log ground normal, sweep hits; check slopes and step-up.

5. **dt spikes / fixed-step starvation**
   - Symptom: buffer/coyote decay too fast.
   - Verify: log `fixed_step_count`, clamp dt, or use fixed timestep accumulator.

## 5) Minimal C++-style pseudocode skeletons

### Player.h (public API)
```cpp
#pragma once

struct PlayerInputCommands {
    Vec2 moveAxis;
    Vec2 lookDelta;
    bool sprintHeld;
    bool crouchHeld;
    bool jumpPressed;   // edge
    bool jumpReleased;  // edge
};

struct PlayerMovementParams {
    float walkSpeed;
    float sprintSpeed;
    float crouchSpeed;
    float accelGround;
    float accelAir;
    float friction;
    float gravity;
    float jumpSpeed;
    float jumpBufferTime;
    float coyoteTime;
};

struct Player {
    void Init();
    void UpdateRender(const InputState& input, float dt);
    void UpdateFixed(const CollisionWorld& col, float fixedDt);

    // Accessors
    Vec3 GetPosition() const;
    Vec3 GetVelocity() const;
    bool IsGrounded() const;

private:
    // Input
    PlayerInputCommands m_cmds;
    bool m_inputFocused;

    // Movement
    Vec3 m_position;
    Vec3 m_velocity;
    bool m_grounded;

    // Jump system
    float m_jumpBufferTime;
    float m_coyoteTime;
    bool m_jumpConsumed;

    PlayerMovementParams m_params;
};
```

### Player_Input.cpp
```cpp
void Player::UpdateRender(const InputState& input, float dt) {
    m_inputFocused = !input.uiCaptured; // or editor focus

    if (!m_inputFocused) {
        // Clear edges, but keep movement zeroed.
        m_cmds = {};
        return;
    }

    m_cmds.moveAxis = input.moveAxis;
    m_cmds.lookDelta = input.lookDelta;
    m_cmds.sprintHeld = input.sprintHeld;
    m_cmds.crouchHeld = input.crouchHeld;

    // Edge capture (render frame domain)
    m_cmds.jumpPressed = input.jumpPressed;   // edge for this frame
    m_cmds.jumpReleased = input.jumpReleased;

    if (m_cmds.jumpPressed) {
        m_jumpBufferTime = m_params.jumpBufferTime; // set buffer
        m_jumpConsumed = false; // new press is eligible
    }
}
```

### Player_Movement.cpp
```cpp
void Player::UpdateFixed(const CollisionWorld& col, float dt) {
    // 1) timers (fixed domain)
    m_jumpBufferTime = max(0.0f, m_jumpBufferTime - dt);
    m_coyoteTime = max(0.0f, m_coyoteTime - dt);

    // 2) detect grounded using collision/sweep
    m_grounded = CheckGrounded(col);
    if (m_grounded) {
        m_coyoteTime = m_params.coyoteTime;
        if (m_cmds.jumpReleased) {
            m_jumpConsumed = false;
        }
    }

    // 3) jump consume
    if (!m_jumpConsumed && m_jumpBufferTime > 0.0f && (m_grounded || m_coyoteTime > 0.0f)) {
        m_velocity.y = m_params.jumpSpeed;
        m_jumpBufferTime = 0.0f;
        m_coyoteTime = 0.0f;
        m_jumpConsumed = true;
        m_grounded = false;
    }

    // 4) movement (wish direction, accel, friction)
    Vec3 wishDir = ComputeWishDir(m_cmds.moveAxis);
    ApplyAcceleration(wishDir, dt, m_grounded ? m_params.accelGround : m_params.accelAir);

    if (m_grounded) {
        ApplyFriction(dt, m_params.friction);
    } else {
        m_velocity.y -= m_params.gravity * dt;
    }

    // 5) sweep/slide + step-up
    MoveAndSlide(col, dt);
}
```

### Ring-buffer logger (for jump failures)
```cpp
struct PlayerDebugFrame {
    float frameDt;
    float fixedDt;
    bool grounded;
    bool jumpPressed;
    bool jumpReleased;
    float jumpBuffer;
    float coyote;
    bool jumpConsumed;
};

class PlayerDebugRing {
public:
    void Push(const PlayerDebugFrame& f) {
        m_frames[m_head] = f;
        m_head = (m_head + 1) % kMax;
        m_count = min(m_count + 1, kMax);
    }

    void Dump() const {
        for (int i = 0; i < m_count; ++i) {
            const auto& f = m_frames[(m_head + i) % kMax];
            LogFrame(f);
        }
    }

private:
    static constexpr int kMax = 120;
    PlayerDebugFrame m_frames[kMax];
    int m_head = 0;
    int m_count = 0;
};
```

## 6) UI/editor input capture handling

- Centralize a `uiCaptured` boolean in the input system.
- When UI is focused, **do not** process gameplay edges.
- When focus returns, clear `jumpPressed/jumpReleased` edges to avoid false triggers.
- Keep movement commands zeroed so the player does not drift while in UI.

---

### Final note
For reliability: always set buffer times on render-frame input, then consume only in fixed update. Never clear buffer on input release; only clear on successful consume or timeout.
