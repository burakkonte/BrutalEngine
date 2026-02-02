#ifndef BRUTAL_CORE_PLATFORM_H
#define BRUTAL_CORE_PLATFORM_H

#include "brutal/core/types.h"

namespace brutal {

enum KeyCode {
    KEY_UNKNOWN = 0,
    KEY_A = 'A', KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_0 = '0', KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SPACE = 0x20,
    KEY_DELETE = 0x2E,
    KEY_ESCAPE = 0x1B,
    KEY_SHIFT = 0x10,
    KEY_CONTROL = 0x11,
    KEY_LCONTROL = 0xA2,  // Left Ctrl specifically
    KEY_RCONTROL = 0xA3,  // Right Ctrl specifically
    KEY_GRAVE = 0xC0,
    KEY_F1 = 0x70, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
};

struct ButtonState {
    bool down;
    bool pressed;
    bool released;
};

struct MouseState {
    i32 x, y;
    i32 delta_x, delta_y;
    i32 raw_dx, raw_dy;
    ButtonState left, right, middle;
    i32 wheel_delta;
};

struct KeyState {
    bool down[256];          // Currently held this frame
    bool down_previous[256]; // Was held last frame
    bool pressed[256];       // Pressed since last poll
    bool released[256];      // Released since last poll
};

struct InputState {
    KeyState keys;
    MouseState mouse;
    bool keyboard_consumed;
    bool mouse_consumed;
};

struct MouseDelta {
    i32 dx;
    i32 dy;
};

struct MouseLookTelemetryFrame {
    u64 frame_index;
    f32 dt;
    f32 frame_ms;
    i32 raw_dx;
    i32 raw_dy;
    i32 consumed_dx;
    i32 consumed_dy;
    f32 yaw_delta;
    f32 pitch_delta;
    bool dt_spike;
    bool dx_spike;
    bool mouse_look_enabled;
    bool ui_mouse_capture;
    bool input_focused;
};

struct MouseLookTelemetry {
    static constexpr u32 kRingSize = 120;
    MouseLookTelemetryFrame frames[kRingSize];
    u32 index;
    u64 frame_index;
    u64 last_dump_frame;
};

struct PlatformState {
    void* hwnd;
    void* hdc;
    void* hglrc;
    i32 window_width;
    i32 window_height;
    bool should_quit;
    bool mouse_captured;
    bool mouse_look_enabled;
    bool input_focused;
    using MessageHandler = bool (*)(void* hwnd, u32 msg, u64 wparam, i64 lparam);
    MessageHandler message_handler;
    InputState input;
    i32 mouse_accum_dx;
    i32 mouse_accum_dy;
    MouseLookTelemetry mouse_look;
};

bool platform_init(PlatformState* state, const char* title, i32 width, i32 height);
void platform_shutdown(PlatformState* state);
void platform_poll_events(PlatformState* state);
void platform_swap_buffers(PlatformState* state);
void platform_set_mouse_capture(PlatformState* state, bool capture);
void platform_enable_mouse_look(PlatformState* state);
void platform_disable_mouse_look(PlatformState* state);
MouseDelta platform_consume_mouse_delta(PlatformState* state);
void platform_clear_mouse_delta(PlatformState* state);
void platform_mouse_look_record(PlatformState* state,
    f32 dt,
    f32 frame_ms,
    i32 raw_dx,
    i32 raw_dy,
    i32 consumed_dx,
    i32 consumed_dy,
    f32 yaw_delta,
    f32 pitch_delta,
    bool ui_mouse_capture);
const MouseLookTelemetryFrame* platform_mouse_look_latest(const PlatformState* state);
void platform_set_window_title(PlatformState* state, const char* title);
void platform_set_message_handler(PlatformState* state, PlatformState::MessageHandler handler);

inline bool platform_key_down(const InputState* input, i32 key) {
    i32 k = key & 0xFF;
    return input->keys.down[k];
}

inline bool platform_key_pressed(const InputState* input, i32 key) {
    i32 k = key & 0xFF;
    return input->keys.pressed[k];
}

inline bool platform_key_released(const InputState* input, i32 key) {
    i32 k = key & 0xFF;
    return input->keys.released[k];
}

inline void platform_input_consume_keyboard(InputState* input) {
    if (!input) return;
    input->keyboard_consumed = true;
    for (int i = 0; i < 256; ++i) {
        input->keys.down[i] = false;
        input->keys.pressed[i] = false;
        input->keys.released[i] = false;
    }
}

inline void platform_input_consume_mouse(InputState* input) {
    if (!input) return;
    input->mouse_consumed = true;
    input->mouse.left.pressed = input->mouse.left.released = false;
    input->mouse.right.pressed = input->mouse.right.released = false;
    input->mouse.middle.pressed = input->mouse.middle.released = false;
    input->mouse.delta_x = input->mouse.delta_y = 0;
    input->mouse.raw_dx = input->mouse.raw_dy = 0;
    input->mouse.wheel_delta = 0;
}

}

#endif
