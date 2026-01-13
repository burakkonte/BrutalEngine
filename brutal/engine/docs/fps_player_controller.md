# FPS Player Controller (Horror FPS) — Main Design Doc

## A) Scope & Non-Goals
**Scope**
- Single target: gothic horror FPS.
- Player controller implementing sweep/slide movement, grounded/air states, and deterministic input consumption.
- Fix intermittent SPACE/jump failures in variable timestep and fixed timestep loops.

**Non-Goals**
- No engine-wide refactor (no ECS rewrite, no general-purpose features).
- No complex multiplayer prediction/reconciliation.
- No third-person systems, parkour, or traversal gadgets.
- No generalized “UE-style” extensibility; keep it lean and specific.

---

## B) File Layout (Tokyo-style split)
Organize player logic by responsibility; a small orchestrator owns shared state and calls each module in a defined order.

**Player.h / Player.cpp (orchestrator)**
- **Responsibilities**:
  - Owns high-level player state (position, velocity, grounded, timers, config).
  - Orchestrates update order (input → timers → ground → jump → move → resolve → post).
  - Defines public API: `Update(dt)`, `FixedUpdate(fixedDt)` if used, `OnInputEvent(...)`, etc.
- **Owns state**:
  - `PlayerConfig` tuning params.
  - `PlayerState` (position, velocity, grounded, jumpBufferTimer, coyoteTimer, etc.).
  - `PlayerInputState` (spaceDown, spacePressed, movement axes, etc.).

**Player_Input.cpp**
- **Responsibilities**:
  - Capture raw input (events/polling) and create deterministic commands/intents.
  - Edge detection for SPACE with debouncing/repeat handling.
  - Gate input based on UI/editor focus (e.g., ImGui).
- **Owns state**:
  - Input tracking (previous state, pressed edge, buffered commands).
  - Jump buffer timer set on pressed edge.

**Player_Movement.cpp**
- **Responsibilities**:
  - Apply acceleration/friction/gravity.
  - Consume jump at a single point of truth.
  - Apply sweep/slide collision and step/ground logic.
- **Owns state**:
  - Movement-specific transient values if needed (ground normal, last step hit, etc.).

**Player_Camera.cpp (optional)**
- **Responsibilities**: camera rotation, head bob, recoil, lean.
- **Owns state**: camera pitch/yaw, bob phase, recoil accumulators.

**Player_Audio.cpp (optional)**
- **Responsibilities**: footsteps, landing sounds, jump sounds.
- **Owns state**: audio timers, surface type tracking.

**Player_Interact.cpp (optional)**
- **Responsibilities**: interact raycasts, prompts.
- **Owns state**: last interact target.

**Player_Flashlight.cpp (optional)**
- **Responsibilities**: toggle/charge logic.
- **Owns state**: battery %, cooldowns.

---

## C) Data & Tuning Parameters
Define all tuning values in one `PlayerConfig`. Prefer ranges, not magic numbers. Keep in a single header or data asset.

**Suggested parameters + ranges** (adjust in tuning pass):
- **Walk speed**: 2.5–4.5 m/s
- **Sprint speed**: 4.5–7.0 m/s
- **Crouch speed**: 1.5–2.5 m/s
- **Acceleration (ground)**: 15–40 m/s²
- **Acceleration (air)**: 5–15 m/s²
- **Friction (ground)**: 6–12
- **Gravity**: 18–30 m/s²
- **Jump height**: 0.9–1.4 m
- **Jump velocity**: derived from height: `v = sqrt(2 * g * h)`
- **Air control scalar**: 0.3–0.6
- **Coyote time**: 0.08–0.2 s
- **Jump buffer**: 0.08–0.2 s
- **Max slope angle**: 35–50 degrees
- **Step height**: 0.2–0.45 m
- **Ground snap distance**: 0.05–0.2 m
- **Max fall speed**: 25–60 m/s
- **Max dt clamp**: 0.05–0.1 s (for timer integration)

---

## D) Input-to-Movement Pipeline (Deterministic)
**Goal**: Never miss a SPACE press even with fixed-step mismatch or variable frame rate.

**Raw input sampling**
- **Event-driven**: On key down/up, store `spaceDown` and detect pressed edge (`spacePressed = spaceDown && !spaceDownPrev`), ignoring OS key repeat.
- **Polling**: Each frame, read current key state, compare to previous to compute `spacePressed`.

**Command generation**
- Convert raw input into `PlayerCommands`:
  - `moveAxis` (normalized vector)
  - `wantSprint`, `wantCrouch`
  - `jumpPressed` (edge only)
  - `jumpHeld`
- `jumpPressed` sets a **jump buffer timer**: `jumpBufferTimer = jumpBufferDuration`.

**Fixed-step mismatch protection**
- Always store jump requests in timers (buffer + coyote).
- The movement step consumes jump if conditions are met.
- **Never rely on a single frame edge** to trigger jump.

**Prevent missing the pressed edge**
- Update input *before* fixed-step consumes.
- For fixed-step: copy commands from render/update frame into a persistent buffer consumed by physics steps.
- Do not reset `jumpPressed` until after it is copied into the command buffer.

---

## E) Update Order (CRITICAL)
**Single Source of Truth**: jump consumption happens only once in `Player_Movement.cpp`.

### Variable Timestep Update Order (per frame)
```text
1) Input update (capture edges; set jumpBufferTimer)
2) Timers update (jumpBufferTimer -= dt; coyoteTimer update)
3) Ground detection update (sweep test / ground probe)
4) Jump consume rule (if buffer + coyote/ground allow)
5) Movement integration (accel, friction, gravity)
6) Sweep/slide resolve
7) Post-move ground snap + slope handling (optional)
```

### Fixed Physics Update (accumulated steps) + Render Update
```text
Render/Frame Update:
  - Poll input
  - Build commands (jumpPressed edge => set jumpBufferTimer)
  - Store commands in a persistent buffer accessible to fixed steps

FixedUpdate Loop (0..N steps):
  1) Use latest command buffer (not raw input)
  2) Timers update (jumpBufferTimer -= fixedDt; coyoteTimer update)
  3) Ground detection update
  4) Jump consume rule
  5) Movement integration
  6) Sweep/slide resolve
  7) Post-move ground snap + slope handling
```

**Key**: Input capture occurs outside the fixed-step; timers persist across steps so a single press is not missed.

---

## F) Correct Jump Design (Fixing Intermittent SPACE)
**Definitions**
- **Jump Buffer**: a timer set when SPACE pressed; allows jump when landing shortly after.
- **Coyote Time**: a timer set when leaving ground; allows jump shortly after stepping off.

**Consume rule (single point of truth)**
Jump is consumed when:
- `jumpBufferTimer > 0` **AND** (`isGrounded` **OR** `coyoteTimer > 0`) **AND** `!jumpConsumedThisFrame`.

**Consume behavior**
- Apply `velocity.y = jumpVelocity`.
- Clear `jumpBufferTimer = 0` and `coyoteTimer = 0`.
- Mark `jumpConsumedThisFrame = true` (reset next frame).

**Prevent double-consume / race conditions**
- Only one function can consume jump (the movement module).
- Do not also consume in input code.
- For fixed-step: clear buffer only in physics step to avoid render-step clearing.

**Hold vs Tap SPACE**
- `jumpPressed` sets buffer once.
- `jumpHeld` can be used for variable jump height (optional) but must not re-trigger jump.

**dt spikes**
- Clamp dt used for timers (e.g., `clampedDt = min(dt, maxDtClamp)`), to avoid emptying buffer on a single spike.

---

## G) UI/Editor Input Capture
When UI or editor owns keyboard focus, gameplay input should be ignored.

**Rules**
- If UI requests capture (e.g., ImGui `WantCaptureKeyboard`), do **not** update gameplay inputs.
- Clear only per-frame input edges (`jumpPressed`) but **do not** clear held states unless you explicitly know focus was lost.
- Avoid permanent blocking by resuming input capture the next frame UI releases focus.
- If focus is lost mid-jump buffer, decide on consistent policy:
  - Recommended: freeze timers while UI captures, or expire them (explicit choice). Do not silently clear buffer.

---

## H) Debug & Telemetry Hooks
**Overlay per frame**
- `spaceDown`, `spacePressed`, `jumpBufferTimer`
- `grounded`, `coyoteTimer`
- `jumpConsumedThisFrame`
- `dt`, `fixedDt`, `stepsThisFrame`

**Frame ring-buffer**
- Store last N frames (N = 120–240).
- Record: input edges, timers, grounded state, velocity, dt, and whether jump was consumed.

**Dump on failure**
- Trigger a dump when `jumpBufferTimer > 0` persists longer than X ms without a consume.
- Log reason flags: `grounded`, `coyote`, `uiCaptured`, `jumpConsumed`.

---

## I) Implementation Checklist
**Priority order**
1. Create file split and orchestrator (Player.h/.cpp + Input/Movement modules).
2. Implement deterministic input edge capture with UI gating.
3. Implement jump buffer + coyote timers in movement step.
4. Implement update order exactly as specified.
5. Add overlay + ring-buffer; add “dump on failure” trigger.
6. Verify behavior in variable timestep and fixed-step.

**Unit-style validation scenarios**
- **Basic jump**: press SPACE, jump triggers immediately.
- **Buffered landing**: press SPACE just before landing → jump triggers on landing.
- **Coyote**: walk off ledge, press SPACE within coyote window → jump triggers.
- **Fixed-step**: low render FPS, multiple physics steps → jump still triggers.
- **UI focus**: when UI captures keyboard, gameplay input ignored, resumes after release.
- **Slopes and steps**: jump on slope within angle limit, no false grounded.
- **Hold vs tap**: holding SPACE does not re-trigger jump.
- **dt spikes**: artificially spike dt; buffer remains reliable.
