#ifndef PLAYGROUND_EDITOR_EDITOR_H
#define PLAYGROUND_EDITOR_EDITOR_H

#include "brutal/core/types.h"
#include "brutal/core/platform.h"
#include "brutal/renderer/camera.h"
#include "brutal/renderer/renderer.h"
#include "brutal/world/scene.h"
#include "brutal/world/player.h"

#include <vector>

namespace brutal {

    struct EditorViewportState {
        Vec2 min;
        Vec2 max;
        Vec2 size;
        bool hovered;
        bool focused;
    };

    enum class EditorSelectionType {
        None,
        Brush,
        Prop,
        Light
    };

    struct EditorSelectionItem {
        EditorSelectionType type;
        u32 index;
    };

    struct EditorGizmoState {
        int operation;
        int mode;
        bool using_gizmo;
    };

    struct EditorFramebuffer {
        u32 fbo;
        u32 color_texture;
        u32 depth_rbo;
        i32 width;
        i32 height;
    };

    struct EditorContext {
        bool active;
        bool dockspace_built;
        bool wants_capture_mouse;
        bool wants_capture_keyboard;

        EditorViewportState viewport;
        EditorFramebuffer scene_buffer;

        Camera camera;
        f32 move_speed;
        f32 look_sensitivity;

        EditorSelectionType selection_type;
        u32 selection_index;
        std::vector<EditorSelectionItem> selection;

        bool show_grid;
        bool rebuild_world;
        bool rebuild_collision;

        EditorGizmoState gizmo;

        bool snap_translate;
        f32 snap_translate_value;
        bool snap_rotate;
        f32 snap_rotate_value;
        bool snap_scale;
        f32 snap_scale_value;
    };

    void editor_init(EditorContext* ctx, PlatformState* platform);
    void editor_shutdown(EditorContext* ctx);
    void editor_set_active(EditorContext* ctx, bool active, PlatformState* platform, Player* player);

    void editor_begin_frame(EditorContext* ctx, PlatformState* platform);
    void editor_update(EditorContext* ctx, Scene* scene, PlatformState* platform, f32 dt);
    void editor_build_ui(EditorContext* ctx, Scene* scene, PlatformState* platform);
    void editor_render_scene(EditorContext* ctx, Scene* scene, RendererState* renderer);
    void editor_end_frame(EditorContext* ctx);

    bool editor_scene_needs_rebuild(const EditorContext* ctx);
    void editor_clear_rebuild_flag(EditorContext* ctx);

}

#endif
