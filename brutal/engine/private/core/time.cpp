#include "brutal/core/time.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>

namespace brutal {

void time_init(TimeState* state) {
    timeBeginPeriod(1);
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    state->frequency = freq.QuadPart;
    state->start_time = now.QuadPart;
    state->last_time = now.QuadPart;
    state->total_time = 0;
    state->timing = {};
}

void time_update(TimeState* state) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    
    f64 dt = static_cast<f64>(now.QuadPart - state->last_time) / state->frequency;
    state->last_time = now.QuadPart;
    state->total_time += dt;
    
    state->timing.delta_time = dt;
    state->timing.total_time = state->total_time;
    state->timing.frame_time_ms = dt * 1000.0;
    state->timing.fps = (dt > 0) ? 1.0 / dt : 0;
}

f64 time_now() {
    static LARGE_INTEGER freq = {};
    static bool init = false;
    if (!init) {
        QueryPerformanceFrequency(&freq);
        init = true;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<f64>(now.QuadPart) / freq.QuadPart;
}

}
