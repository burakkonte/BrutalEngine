#include "debug_camera.h"

namespace brutal {

    void debug_free_camera_init(DebugFreeCamera* cam) {
        if (!cam) return;
        camera_init(&cam->camera);
        cam->camera.position = Vec3(0.0f, 2.0f, 8.0f);
        cam->move_speed = 7.5f;
        cam->look_sensitivity = 0.0025f;
    }

    void debug_free_camera_update(DebugFreeCamera* cam, const InputState* input, f32 dt) {
        if (!cam || !input) return;
        if (input->mouse.right.down) {
            const f32 dx = static_cast<f32>(input->mouse.raw_dx);
            const f32 dy = static_cast<f32>(input->mouse.raw_dy);
            camera_rotate(&cam->camera, -dx * cam->look_sensitivity, -dy * cam->look_sensitivity);
        }

        Vec3 move(0.0f, 0.0f, 0.0f);
        if (platform_key_down(input, KEY_W)) move = move + camera_forward(&cam->camera);
        if (platform_key_down(input, KEY_S)) move = move - camera_forward(&cam->camera);
        if (platform_key_down(input, KEY_D)) move = move + camera_right(&cam->camera);
        if (platform_key_down(input, KEY_A)) move = move - camera_right(&cam->camera);
        if (platform_key_down(input, KEY_SPACE)) move.y += 1.0f;
        if (platform_key_down(input, KEY_CONTROL)) move.y -= 1.0f;

        if (vec3_length(move) > 0.001f) {
            move = vec3_normalize(move);
            cam->camera.position = cam->camera.position + move * (cam->move_speed * dt);
        }
    }

}
