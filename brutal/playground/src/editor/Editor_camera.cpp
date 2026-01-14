#include "editor/Editor_camera.h"

#include "brutal/renderer/camera.h"

namespace brutal {

    void editor_camera_update(EditorContext* ctx, const PlatformState* platform, f32 dt) {
        if (!ctx || !platform) return;
        if (!ctx->viewport.hovered || ctx->wants_capture_mouse) return;

        const InputState& input = platform->input;
        const bool looking = input.mouse.right.down;

        if (looking) {
            const f32 dx = static_cast<f32>(input.mouse.raw_dx);
            const f32 dy = static_cast<f32>(input.mouse.raw_dy);
            camera_rotate(&ctx->camera, -dx * ctx->look_sensitivity, -dy * ctx->look_sensitivity);
        }

        Vec3 move(0.0f, 0.0f, 0.0f);
        if (platform_key_down(&input, KEY_W)) move = move + camera_forward(&ctx->camera);
        if (platform_key_down(&input, KEY_S)) move = move - camera_forward(&ctx->camera);
        if (platform_key_down(&input, KEY_D)) move = move + camera_right(&ctx->camera);
        if (platform_key_down(&input, KEY_A)) move = move - camera_right(&ctx->camera);
        if (platform_key_down(&input, KEY_SPACE)) move.y += 1.0f;
        if (platform_key_down(&input, KEY_CONTROL)) move.y -= 1.0f;

        if (vec3_length(move) > 0.001f) {
            move = vec3_normalize(move);
            ctx->camera.position = ctx->camera.position + move * (ctx->move_speed * dt);
        }
    }

}
