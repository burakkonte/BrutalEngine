#ifndef BRUTAL_CORE_PROFILER_H
#define BRUTAL_CORE_PROFILER_H

#include "brutal/core/types.h"

namespace brutal {

    struct ProfileEntry {
        const char* name;
        f64 ms;
        u32 depth;
    };

    struct FrameProfile {
        u32 count;
        ProfileEntry entries[64];
        f64 frame_ms;
    };

#if defined(BRUTAL_ENABLE_PROFILER) && BRUTAL_ENABLE_PROFILER
    void profiler_init();
    void profiler_shutdown();
    void profiler_begin_frame();
    void profiler_end_frame();
    const FrameProfile* profiler_get_frame();

    class ProfileScope {
    public:
        explicit ProfileScope(const char* name);
        ~ProfileScope();

        ProfileScope(const ProfileScope&) = delete;
        ProfileScope& operator=(const ProfileScope&) = delete;
    };

#define PROFILE_SCOPE(name) ::brutal::ProfileScope brutal_profile_scope_##__LINE__(name)
#else
    inline void profiler_init() {}
    inline void profiler_shutdown() {}
    inline void profiler_begin_frame() {}
    inline void profiler_end_frame() {}
    inline const FrameProfile* profiler_get_frame() { return nullptr; }

    class ProfileScope {
    public:
        explicit ProfileScope(const char*) {}
    };

#define PROFILE_SCOPE(name) ((void)0)
#endif

}

#endif
