#include "brutal/core/profiler.h"

#if defined(BRUTAL_ENABLE_PROFILER) && BRUTAL_ENABLE_PROFILER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace brutal {

    struct ProfilerState {
        i64 frequency;
        i64 frame_start;
        FrameProfile frame;
        u32 stack_count;
        i64 stack_start[64];
        const char* stack_name[64];
    };

    static ProfilerState g_profiler = {};

    static f64 ticks_to_ms(i64 ticks) {
        return (static_cast<f64>(ticks) / static_cast<f64>(g_profiler.frequency)) * 1000.0;
    }

    void profiler_init() {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        g_profiler.frequency = freq.QuadPart;
        g_profiler.frame = {};
        g_profiler.stack_count = 0;
    }

    void profiler_shutdown() {
        g_profiler = {};
    }

    void profiler_begin_frame() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        g_profiler.frame_start = now.QuadPart;
        g_profiler.frame.count = 0;
        g_profiler.frame.frame_ms = 0.0;
        g_profiler.stack_count = 0;
    }

    static void profiler_push(const char* name) {
        if (g_profiler.stack_count >= 64) return;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        g_profiler.stack_start[g_profiler.stack_count] = now.QuadPart;
        g_profiler.stack_name[g_profiler.stack_count] = name;
        g_profiler.stack_count++;
    }

    static void profiler_pop() {
        if (g_profiler.stack_count == 0) return;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        g_profiler.stack_count--;
        i64 start = g_profiler.stack_start[g_profiler.stack_count];
        const char* name = g_profiler.stack_name[g_profiler.stack_count];

        if (g_profiler.frame.count >= 64) return;
        ProfileEntry& entry = g_profiler.frame.entries[g_profiler.frame.count++];
        entry.name = name;
        entry.ms = ticks_to_ms(now.QuadPart - start);
        entry.depth = g_profiler.stack_count;
    }

    void profiler_end_frame() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        g_profiler.frame.frame_ms = ticks_to_ms(now.QuadPart - g_profiler.frame_start);

        if (g_profiler.frame.count < 64) {
            ProfileEntry& entry = g_profiler.frame.entries[g_profiler.frame.count++];
            entry.name = "Frame";
            entry.ms = g_profiler.frame.frame_ms;
            entry.depth = 0;
        }
    }

    const FrameProfile* profiler_get_frame() {
        return &g_profiler.frame;
    }

    ProfileScope::ProfileScope(const char* name) {
        profiler_push(name);
    }

    ProfileScope::~ProfileScope() {
        profiler_pop();
    }

}

#endif
