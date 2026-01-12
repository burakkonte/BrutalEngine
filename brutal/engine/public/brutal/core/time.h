#ifndef BRUTAL_CORE_TIME_H
#define BRUTAL_CORE_TIME_H

#include "brutal/core/types.h"

namespace brutal {

struct FrameTiming {
    f64 delta_time;
    f64 total_time;
    f64 fps;
    f64 frame_time_ms;
};

struct TimeState {
    i64 frequency;
    i64 start_time;
    i64 last_time;
    f64 total_time;
    FrameTiming timing;
};

void time_init(TimeState* state);
void time_update(TimeState* state);
f64 time_now();  // Get current time in seconds

}

#endif
