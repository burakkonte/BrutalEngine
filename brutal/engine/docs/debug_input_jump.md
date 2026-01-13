# Debugging Intermittent SPACE / Jump Issues

## Symptom Classification
First determine which layer is failing:

**A) Input edge missing**
- `spaceDown` changes, but `spacePressed` never becomes true.
- `jumpBufferTimer` never set.

**B) Movement consume failing**
- `spacePressed` and `jumpBufferTimer` are set, but jump never occurs.
- `jumpBufferTimer` expires with no consume.

---

## Fast Tests (Isolate Cause)
1. **Overlay check**
   - Press SPACE repeatedly.
   - Expected: `spacePressed` flashes for one frame; `jumpBufferTimer` refreshes.

2. **Fixed-step mismatch check**
   - Drop render FPS; confirm `jumpBufferTimer` still exists in fixed steps.
   - Expected: jump consumes on next grounded/coyote opportunity.

3. **UI focus check**
   - Enable UI or editor panel.
   - Expected: input ignored while `WantCaptureKeyboard` is true, resumes immediately after.

4. **Auto-repeat check**
   - Hold SPACE.
   - Expected: `spacePressed` only fires once on the initial down.

---

## Common Root Causes
- **Pressed edge reset timing wrong** (cleared before fixed step reads it).
- **WM_KEYDOWN auto-repeat treated as pressed** (false positives or timing noise).
- **Message pump order** (input events processed after physics step).
- **Fixed-step mismatch** (buffer not persisted across steps).
- **UI capturing keyboard** (input ignored unexpectedly).
- **dt spike consuming buffer** (timer drops to 0 in one frame).

---

## Step-by-Step Debugging Procedure
1) **Add overlay metrics**
- Show: `spaceDown`, `spacePressed`, `jumpBufferTimer`, `grounded`, `coyoteTimer`, `jumpConsumedThisFrame`, `dt`.

2) **Add ring buffer**
- Store last 120–240 frames with all key flags and timers.

3) **Reproduce + dump**
- When jump requested but not consumed within X ms, dump ring buffer to log.

4) **Interpret dump patterns**
- **Pattern A**: `spacePressed=0` always, `spaceDown` toggles → input edge detection bug.
- **Pattern B**: `jumpBufferTimer` set, but `grounded=0` and `coyoteTimer=0` → ground detection or coyote update bug.
- **Pattern C**: `jumpBufferTimer` set in render frame, but fixed step sees 0 → command buffer not persisted.
- **Pattern D**: `jumpBufferTimer` instantly drops from >0 to 0 on dt spike → timer clamp missing.
- **Pattern E**: `uiCaptured=1` during press → input correctly ignored; fix UI focus state if wrong.

---

## Reference Implementation (Most Reliable)
### Input edge capture (event or polling)
```cpp
// Called once per render/frame update
void PlayerInput::Update(const RawInput& raw, bool uiCaptured) {
    spacePressed = false;
    if (uiCaptured) {
        // Ignore gameplay input edges while UI is capturing
        return;
    }

    bool newSpaceDown = raw.IsKeyDown(KEY_SPACE);
    spacePressed = newSpaceDown && !spaceDown;
    spaceDown = newSpaceDown;

    if (spacePressed) {
        jumpBufferTimer = config.jumpBufferSeconds;
    }
}
```

### Timer integration + consume rule
```cpp
void PlayerMovement::Update(PlayerState& s, const PlayerConfig& c, float dt) {
    float clampedDt = std::min(dt, c.maxDtClamp);

    // 1) Timers
    s.jumpBufferTimer = std::max(0.0f, s.jumpBufferTimer - clampedDt);
    if (s.isGrounded) {
        s.coyoteTimer = c.coyoteSeconds;
    } else {
        s.coyoteTimer = std::max(0.0f, s.coyoteTimer - clampedDt);
    }

    // 2) Consume rule
    bool canJump = (s.jumpBufferTimer > 0.0f) &&
                   (s.isGrounded || s.coyoteTimer > 0.0f) &&
                   !s.jumpConsumedThisFrame;

    if (canJump) {
        s.velocity.y = c.jumpVelocity;
        s.jumpBufferTimer = 0.0f;
        s.coyoteTimer = 0.0f;
        s.jumpConsumedThisFrame = true;
    }
}
```