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
    ButtonState left, right, middle;
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
};

struct PlatformState {
    void* hwnd;
    void* hdc;
    void* hglrc;
    i32 window_width;
    i32 window_height;
    bool should_quit;
    bool mouse_captured;
    InputState input;
};

bool platform_init(PlatformState* state, const char* title, i32 width, i32 height);
void platform_shutdown(PlatformState* state);
void platform_poll_events(PlatformState* state);
void platform_swap_buffers(PlatformState* state);
void platform_set_mouse_capture(PlatformState* state, bool capture);
void platform_set_window_title(PlatformState* state, const char* title);

inline bool platform_key_down(const InputState* input, i32 key) {
    i32 k = key & 0xFF;
    return input->keys.pressed[k];
}

inline bool platform_key_pressed(const InputState* input, i32 key) {
    i32 k = key & 0xFF;
    return input->keys.down[k] && !input->keys.down_previous[k];
}

inline bool platform_key_released(const InputState* input, i32 key) {
    i32 k = key & 0xFF;
    return input->keys.released[k];
}

}

#endif
