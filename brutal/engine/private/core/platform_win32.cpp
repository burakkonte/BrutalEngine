#include "brutal/core/platform.h"
#include "brutal/core/logging.h"
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace brutal {

static PlatformState* g_platform = nullptr;

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
                    platform_set_mouse_capture(g_platform, false);
                } else {
                    g_platform->should_quit = true;
                }
            }
            if (msg == WM_SYSKEYDOWN) break;
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
    
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    if (state->mouse_captured) {
        POINT center;
        RECT rect;
        GetClientRect((HWND)state->hwnd, &rect);
        center.x = rect.right / 2;
        center.y = rect.bottom / 2;
        
        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient((HWND)state->hwnd, &cursor);
        
        state->input.mouse.delta_x = cursor.x - center.x;
        state->input.mouse.delta_y = cursor.y - center.y;
        
        ClientToScreen((HWND)state->hwnd, &center);
        SetCursorPos(center.x, center.y);
    }
    
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
    ShowCursor(!capture);
    if (capture) {
        SetCapture((HWND)state->hwnd);
    } else {
        ReleaseCapture();
    }
}

void platform_set_window_title(PlatformState* state, const char* title) {
    SetWindowTextA((HWND)state->hwnd, title);
}
void platform_set_message_handler(PlatformState* state, PlatformState::MessageHandler handler) {
    state->message_handler = handler;
}

}
