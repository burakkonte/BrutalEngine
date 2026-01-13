#include "brutal/core/platform.h"
#include "brutal/core/logging.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace brutal {

static PlatformState* g_platform = nullptr;

static bool register_raw_input(HWND hwnd) {
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = 0;
    rid.hwndTarget = hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        LOG_ERROR("Failed to register raw mouse input");
        return false;
    }
    return true;
}

static RECT client_rect_to_screen(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    POINT top_left = { rect.left, rect.top };
    POINT bottom_right = { rect.right, rect.bottom };
    ClientToScreen(hwnd, &top_left);
    ClientToScreen(hwnd, &bottom_right);
    rect.left = top_left.x;
    rect.top = top_left.y;
    rect.right = bottom_right.x;
    rect.bottom = bottom_right.y;
    return rect;
}

static void platform_dump_mouse_look(const PlatformState* state, const char* reason) {
    LOG_WARN("==== Mouse Look Telemetry Dump (%s) ====", reason);
    FILE* file = fopen("mouse_telemetry_dump.csv", "w");
    if (file) {
        fprintf(file,
            "frame,dt,frame_ms,raw_dx,raw_dy,consumed_dx,consumed_dy,"
            "yaw_delta,pitch_delta,dt_spike,dx_spike,mouse_look_enabled,ui_mouse_capture,input_focused\n");
    }
    for (u32 i = 0; i < MouseLookTelemetry::kRingSize; ++i) {
        u32 idx = (state->mouse_look.index + i) % MouseLookTelemetry::kRingSize;
        const MouseLookTelemetryFrame& f = state->mouse_look.frames[idx];
        LOG_WARN("F%llu dt=%.4f raw(%d,%d) consumed(%d,%d) yaw=%.4f pitch=%.4f dtSpike=%d dxSpike=%d look=%d ui=%d focus=%d",
            (unsigned long long)f.frame_index,
            f.dt,
            f.raw_dx,
            f.raw_dy,
            f.consumed_dx,
            f.consumed_dy,
            f.yaw_delta,
            f.pitch_delta,
            f.dt_spike ? 1 : 0,
            f.dx_spike ? 1 : 0,
            f.mouse_look_enabled ? 1 : 0,
            f.ui_mouse_capture ? 1 : 0,
            f.input_focused ? 1 : 0);
        if (file) {
            fprintf(file, "%llu,%.6f,%.3f,%d,%d,%d,%d,%.6f,%.6f,%d,%d,%d,%d,%d\n",
                (unsigned long long)f.frame_index,
                f.dt,
                f.frame_ms,
                f.raw_dx,
                f.raw_dy,
                f.consumed_dx,
                f.consumed_dy,
                f.yaw_delta,
                f.pitch_delta,
                f.dt_spike ? 1 : 0,
                f.dx_spike ? 1 : 0,
                f.mouse_look_enabled ? 1 : 0,
                f.ui_mouse_capture ? 1 : 0,
                f.input_focused ? 1 : 0);
        }
    }
    if (file) {
        fclose(file);
    }
    LOG_WARN("==== End Mouse Look Telemetry Dump ====");
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (!g_platform) return DefWindowProcA(hwnd, msg, wp, lp);
    if (g_platform->message_handler) {
        g_platform->message_handler(hwnd, msg, (u64)wp, (i64)lp);
    }
    
    switch (msg) {
        case WM_CLOSE:
        case WM_QUIT:
            g_platform->should_quit = true;
            return 0;
        case WM_SIZE:
            g_platform->window_width = LOWORD(lp);
            g_platform->window_height = HIWORD(lp);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wp == VK_ESCAPE) {
                if (g_platform->mouse_captured) {
                    platform_disable_mouse_look(g_platform);
                } else {
                    g_platform->should_quit = true;
                }
            }
            if (msg == WM_SYSKEYDOWN) break;
            return 0;
        case WM_INPUT: {
            if (!g_platform->input_focused) {
                return 0;
            }
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lp, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
            if (size == 0) {
                return 0;
            }
            static std::vector<BYTE> buffer;
            buffer.resize(size);
            if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, buffer.data(), &size,
                sizeof(RAWINPUTHEADER)) != size) {
                return 0;
            }
            const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
            if (raw->header.dwType == RIM_TYPEMOUSE) {
                const RAWMOUSE& mouse = raw->data.mouse;
                g_platform->mouse_accum_dx += mouse.lLastX;
                g_platform->mouse_accum_dy += mouse.lLastY;
            }
            return 0;
        }
        case WM_ACTIVATE: {
            bool active = (LOWORD(wp) != WA_INACTIVE);
            g_platform->input_focused = active;
            if (!active) {
                platform_disable_mouse_look(g_platform);
                platform_clear_mouse_delta(g_platform);
            }
            return 0;
        }
        case WM_SETFOCUS:
            g_platform->input_focused = true;
            platform_clear_mouse_delta(g_platform);
            return 0;
        case WM_KILLFOCUS:
            g_platform->input_focused = false;
            platform_disable_mouse_look(g_platform);
            platform_clear_mouse_delta(g_platform);
            return 0;

        case WM_LBUTTONDOWN:
            g_platform->input.mouse.left.down = true;
            g_platform->input.mouse.left.pressed = true;
            return 0;
        case WM_LBUTTONUP:
            g_platform->input.mouse.left.down = false;
            g_platform->input.mouse.left.released = true;
            return 0;
        case WM_RBUTTONDOWN:
            g_platform->input.mouse.right.down = true;
            g_platform->input.mouse.right.pressed = true;
            return 0;
        case WM_RBUTTONUP:
            g_platform->input.mouse.right.down = false;
            g_platform->input.mouse.right.released = true;
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

bool platform_init(PlatformState* state, const char* title, i32 width, i32 height) {
    g_platform = state;
    memset(state, 0, sizeof(*state));
    
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName = "BrutalEngine";
    RegisterClassA(&wc);
    
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    HWND hwnd = CreateWindowA("BrutalEngine", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              rect.right - rect.left, rect.bottom - rect.top,
                              nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) {
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    HDC hdc = GetDC(hwnd);
    
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    
    int format = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, format, &pfd);
    
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);
    
    state->hwnd = hwnd;
    state->hdc = hdc;
    state->hglrc = hglrc;
    state->window_width = width;
    state->window_height = height;
    state->input_focused = true;

    if (!register_raw_input(hwnd)) {
        return false;
    }
    
    LOG_INFO("Platform initialized: %dx%d", width, height);
    return true;
}

void platform_shutdown(PlatformState* state) {
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext((HGLRC)state->hglrc);
    ReleaseDC((HWND)state->hwnd, (HDC)state->hdc);
    DestroyWindow((HWND)state->hwnd);
    g_platform = nullptr;
}

void platform_poll_events(PlatformState* state) {
    memcpy(state->input.keys.down_previous, state->input.keys.down, 
           sizeof(state->input.keys.down_previous));
    
    for (int i = 0; i < 256; i++) {
        SHORT key_state = GetAsyncKeyState(i);
        bool is_down = (key_state & 0x8000) != 0;
        bool was_down = state->input.keys.down_previous[i];

        state->input.keys.down[i] = is_down;
        state->input.keys.pressed[i] = (is_down && !was_down);
        state->input.keys.released[i] = (!is_down && was_down);
    }
    
    state->input.mouse.left.pressed = state->input.mouse.left.released = false;
    state->input.mouse.right.pressed = state->input.mouse.right.released = false;
    state->input.mouse.delta_x = state->input.mouse.delta_y = 0;
    state->input.mouse.raw_dx = state->input.mouse.raw_dy = 0;
    
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    state->input.mouse.raw_dx = state->mouse_accum_dx;
    state->input.mouse.raw_dy = state->mouse_accum_dy;
    
    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient((HWND)state->hwnd, &cursor);
    state->input.mouse.x = cursor.x;
    state->input.mouse.y = cursor.y;
}

void platform_swap_buffers(PlatformState* state) {
    SwapBuffers((HDC)state->hdc);
}

void platform_set_mouse_capture(PlatformState* state, bool capture) {
    state->mouse_captured = capture;
    ShowCursor(capture ? FALSE : TRUE);
    HWND hwnd = (HWND)state->hwnd;
    if (capture) {
        SetCapture(hwnd);
        RECT rect = client_rect_to_screen(hwnd);
        ClipCursor(&rect);
    } else {
        ReleaseCapture();
        ClipCursor(nullptr);
    }
}

void platform_set_window_title(PlatformState* state, const char* title) {
    SetWindowTextA((HWND)state->hwnd, title);
}
void platform_set_message_handler(PlatformState* state, PlatformState::MessageHandler handler) {
    state->message_handler = handler;
}

}

void platform_enable_mouse_look(PlatformState* state) {
    if (!state) return;
    state->mouse_look_enabled = true;
    platform_set_mouse_capture(state, true);
    platform_clear_mouse_delta(state);
}

void platform_disable_mouse_look(PlatformState* state) {
    if (!state) return;
    state->mouse_look_enabled = false;
    platform_set_mouse_capture(state, false);
    platform_clear_mouse_delta(state);
}

MouseDelta platform_consume_mouse_delta(PlatformState* state) {
    MouseDelta delta{ 0, 0 };
    if (!state) return delta;
    delta.dx = state->mouse_accum_dx;
    delta.dy = state->mouse_accum_dy;
    state->mouse_accum_dx = 0;
    state->mouse_accum_dy = 0;
    state->input.mouse.delta_x = delta.dx;
    state->input.mouse.delta_y = delta.dy;
    return delta;
}

void platform_clear_mouse_delta(PlatformState* state) {
    if (!state) return;
    state->mouse_accum_dx = 0;
    state->mouse_accum_dy = 0;
    state->input.mouse.delta_x = 0;
    state->input.mouse.delta_y = 0;
    state->input.mouse.raw_dx = 0;
    state->input.mouse.raw_dy = 0;
}

void platform_mouse_look_record(PlatformState* state,
    f32 dt,
    f32 frame_ms,
    i32 raw_dx,
    i32 raw_dy,
    i32 consumed_dx,
    i32 consumed_dy,
    f32 yaw_delta,
    f32 pitch_delta,
    bool ui_mouse_capture) {
    if (!state) return;
    constexpr f32 kDtSpikeThreshold = 0.05f;
    constexpr i32 kDxSpikeThreshold = 800;
    constexpr u32 kDumpCooldownFrames = 30;

    MouseLookTelemetry& telemetry = state->mouse_look;
    MouseLookTelemetryFrame& frame = telemetry.frames[telemetry.index];
    frame.frame_index = telemetry.frame_index++;
    frame.dt = dt;
    frame.frame_ms = frame_ms;
    frame.raw_dx = raw_dx;
    frame.raw_dy = raw_dy;
    frame.consumed_dx = consumed_dx;
    frame.consumed_dy = consumed_dy;
    frame.yaw_delta = yaw_delta;
    frame.pitch_delta = pitch_delta;
    frame.mouse_look_enabled = state->mouse_look_enabled;
    frame.ui_mouse_capture = ui_mouse_capture;
    frame.input_focused = state->input_focused;

    frame.dt_spike = dt > kDtSpikeThreshold;
    frame.dx_spike = (std::max(std::abs(raw_dx), std::abs(raw_dy)) > kDxSpikeThreshold);

    telemetry.index = (telemetry.index + 1) % MouseLookTelemetry::kRingSize;

    if ((frame.dt_spike || frame.dx_spike) &&
        (frame.frame_index - telemetry.last_dump_frame > kDumpCooldownFrames)) {
        telemetry.last_dump_frame = frame.frame_index;
        platform_dump_mouse_look(state, frame.dt_spike ? "dt spike" : "dx spike");
    }
}

const MouseLookTelemetryFrame* platform_mouse_look_latest(const PlatformState* state) {
    if (!state) return nullptr;
    const MouseLookTelemetry& telemetry = state->mouse_look;
    u32 last_index = (telemetry.index + MouseLookTelemetry::kRingSize - 1) % MouseLookTelemetry::kRingSize;
    return &telemetry.frames[last_index];
}
