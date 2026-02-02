#ifndef PLAYGROUND_DEBUG_CAMERA_H
#define PLAYGROUND_DEBUG_CAMERA_H

#include "brutal/renderer/camera.h"
#include "brutal/core/platform.h"

namespace brutal {

    struct DebugFreeCamera {
        Camera camera;
        f32 move_speed;
        f32 look_sensitivity;
    };

    void debug_free_camera_init(DebugFreeCamera* cam);
    void debug_free_camera_update(DebugFreeCamera* cam, const InputState* input, f32 dt);

}

#endif
