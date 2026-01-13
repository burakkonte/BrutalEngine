# Smooth FPS Mouse Look on Win32: Root Causes, Debug Plan, and Reference Implementation

This guide is a practical, implementation-ready plan to eliminate stuttery mouse look in a custom Win32 FPS engine. It focuses on **raw input**, consistent frame pacing, robust capture, and data-driven debugging.

---

## 1) Likely Root Causes of Stuttery Mouse Look

### A) Absolute cursor position + warping/recenter
* **Symptoms**: Random micro-stutters or “snap” feeling during rotation; smoothness varies by FPS or when the cursor hits window borders.
* **Cause**: `SetCursorPos`/warping interacts with `WM_MOUSEMOVE` (absolute) and produces discontinuities, double-counted deltas, or non-deterministic coalescing.
* **Fix**: Prefer **Raw Input (WM_INPUT)** relative deltas. If warping is used, disable/ignore `WM_MOUSEMOVE` and any absolute position-derived deltas.

### B) Mixing `WM_MOUSEMOVE` + `SetCursorPos`/`ClipCursor`
* **Symptoms**: “sawtooth” deltas, or extra spikes right after warping.
* **Cause**: Windows posts `WM_MOUSEMOVE` for both physical and programmatic cursor moves. If you read deltas from `WM_MOUSEMOVE` and also warp, the warp is interpreted as movement.
* **Fix**: Use **either** raw input or absolute mouse messages, never both. If raw input is used, ignore `WM_MOUSEMOVE` entirely.

### C) Windows “Enhanced pointer precision” (EPP)
* **Symptoms**: Acceleration that feels inconsistent or speed-dependent.
* **Cause**: `WM_MOUSEMOVE` and standard cursor deltas are affected by EPP.
* **Fix**: Use **Raw Input** to bypass EPP. Raw input delivers **unaccelerated** counts from the device.

### D) dt spikes, frame pacing, vsync behavior
* **Symptoms**: Stutter that correlates with frame hitches, loading, or GC; look is smooth only at certain framerates.
* **Cause**: Mouse deltas are applied once per frame; if frame time spikes or is inconsistent, the deltas get clumped.
* **Fix**: Stabilize frame pacing; detect spikes; clamp `dt` for camera updates; avoid mixing input consumption with variable-rate update loops in unpredictable ways.

### E) Fixed-step mismatch (physics tick vs render)
* **Symptoms**: Feels “choppy” despite stable FPS; smoothness worse at certain tick rates.
* **Cause**: Applying mouse deltas in a fixed-step loop while input arrives per-frame, or interpolating incorrectly.
* **Fix**: Apply mouse deltas in **render update** (once per frame), not in physics fixed update. Use physics only for movement that needs fixed integration.

### F) UI/editor capture (ImGui/Editor)
* **Symptoms**: Look randomly stops or becomes sluggish when UI is active.
* **Cause**: UI system captures mouse, blocking your input path, or you’re toggling capture state frequently.
* **Fix**: Gate camera look on UI capture (e.g., `if (ImGui::GetIO().WantCaptureMouse) return;`) and re-lock the cursor only when gameplay mode is active.

---

## 2) Deterministic Debugging Plan + Telemetry

### Per-frame telemetry (log + ring buffer)
Log these values **every frame** into a ring buffer (e.g., 120 frames):
* `frameIndex`, `dt`, `frameTimeMs`
* `rawDx`, `rawDy` (from raw input)
* `processedDx`, `processedDy` (after sensitivity, filters)
* `yawDelta`, `pitchDelta`
* `accumulatedDx/Dy` (before consumption)

### Min/Max and spike detection
Over the last N frames, compute:
* `min/max rawDx/rawDy`
* `min/max dt`
* `mean/variance` of dt

### Micro-stutter detection trigger
Trigger a log dump on:
* `dt > meanDt + 2 * stdDevDt` (or > 2x median)
* `abs(rawDx) > rawDxThreshold` (e.g., > 2x 95th percentile)

### Dump-on-stutter
When triggered, dump last 120 frames to a file (`mouse_telemetry_dump.csv`) with the metrics above. This makes the issue reproducible and inspectable.

---

## 3) Gold Standard Win32 Input Approach

**Use Raw Input (`WM_INPUT`) for relative deltas**.

### Register raw input
```cpp
RAWINPUTDEVICE rid;
rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
rid.usUsage = 0x02;     // HID_USAGE_GENERIC_MOUSE
rid.dwFlags = RIDEV_INPUTSINK; // or 0 for active-only
rid.hwndTarget = hwnd;
RegisterRawInputDevices(&rid, 1, sizeof(rid));
```

### Process `WM_INPUT`
```cpp
case WM_INPUT: {
    UINT size = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

    static std::vector<BYTE> buffer;
    buffer.resize(size);

    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == size) {
        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            const RAWMOUSE& mouse = raw->data.mouse;
            LONG dx = mouse.lLastX;
            LONG dy = mouse.lLastY;

            // Accumulate safely for this frame
            gMouseAccumX.fetch_add(dx, std::memory_order_relaxed);
            gMouseAccumY.fetch_add(dy, std::memory_order_relaxed);
        }
    }
    return 0;
}
```

### Consume once per frame
```cpp
Vec2 ConsumeMouseDelta() {
    const float dx = static_cast<float>(gMouseAccumX.exchange(0));
    const float dy = static_cast<float>(gMouseAccumY.exchange(0));
    return { dx, dy };
}
```

### Focus loss / re-acquire
* On `WM_KILLFOCUS`, stop applying input, release ClipCursor, and stop reading deltas.
* On `WM_SETFOCUS`, re-register raw input if needed and re-lock cursor if in gameplay mode.

---

## 4) Robust Cursor Lock/Capture Strategy

**Recommended:**
1. Hide cursor (ShowCursor(FALSE)).
2. Clip cursor to client rect **only in gameplay mode**.
3. Use raw input for deltas; ignore absolute cursor position.

### Avoid warping (SetCursorPos)
* Warping introduces discontinuities and can trigger `WM_MOUSEMOVE` with synthetic motion.
* It can also fight with OS cursor acceleration and DPI scaling.

### Windowed vs fullscreen
* **Windowed**: Clip to client rect in screen coordinates.
* **Fullscreen**: Clip is optional; raw input doesn’t require the cursor to stay within bounds.

### Multi-monitor
* Use `ClientToScreen` to convert client rect to screen rect before calling `ClipCursor`.

---

## 5) Reference Implementation (Minimal C++ Skeleton)

### Core input loop
```cpp
// globals
std::atomic<long> gMouseAccumX{0};
std::atomic<long> gMouseAccumY{0};

// per-frame
void UpdateCamera(float dt) {
    if (!gGameplayMode || gUiCapturesMouse) {
        ConsumeMouseDelta(); // clear
        return;
    }

    Vec2 delta = ConsumeMouseDelta();

    // Optional smoothing
    if (gEnableSmoothing) {
        delta = SmoothDelta(delta);
    }

    const float sensitivity = gMouseSensitivity;
    float yawDelta = delta.x * sensitivity;
    float pitchDelta = delta.y * sensitivity;

    gYaw += yawDelta;
    gPitch = Clamp(gPitch + pitchDelta, gMinPitch, gMaxPitch);
}
```

### Optional smoothing (low-latency)
```cpp
Vec2 SmoothDelta(Vec2 input) {
    // Exponential smoothing
    // alpha ~ 0.2 - 0.35 keeps low latency
    static Vec2 smoothed{0, 0};
    const float alpha = gSmoothingAlpha;
    smoothed = smoothed * (1.0f - alpha) + input * alpha;
    return smoothed;
}
```

**Tradeoff**: smoothing adds a small delay. Keep it optional and configurable.

### dt handling
* **Typical FPS input**: do **not** scale raw mouse delta by `dt`.
* If you must scale (e.g., converting counts to “units/sec”), clamp `dt` and be consistent.

---

## 6) Actionable Checklist

### Do this first (quick fixes)
1. Switch to **Raw Input (WM_INPUT)** exclusively.
2. Stop using `WM_MOUSEMOVE` and cursor warping for camera deltas.
3. Ensure camera look consumes mouse deltas **once per render frame**.
4. Log `dx/dy`, `dt`, and detect spikes (dump on stutter).

### Verification tests
* **High DPI fast swipe**: no hitching or spikes.
* **Slow micro-aim**: consistent incremental motion, no jitter.
* **Alt-tab**: camera stops when unfocused, resumes cleanly.
* **UI capture**: editor toggles should cleanly disable/enable look.
* **144/240Hz**: stable deltas and no frame-dependent feel.

---

## Practical Notes
* Raw input is the most consistent and AAA-feeling path for FPS look on Win32.
* If you still see stutter, it’s usually **frame pacing**, **thread contention**, or **UI capture toggling**.
* Telemetry is your fastest path to a deterministic root-cause.
