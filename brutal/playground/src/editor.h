#ifndef PLAYGROUND_EDITOR_H
#define PLAYGROUND_EDITOR_H

#include "brutal/core/types.h"
#include "brutal/renderer/camera.h"
#include "brutal/world/scene.h"

namespace brutal {

    struct PlatformState;
    struct Player;
    struct RendererState;
    struct EditorState {
        bool active;
        Camera camera;
        f32 move_speed;
        f32 look_sensitivity;

        enum class SelectionType {
            None,
            Brush,
            Prop,
            Light
        };

        SelectionType selection_type;
        u32 selection_index;

        enum class GizmoMode {
            Translate,
            Rotate,
            Scale
        };

        GizmoMode gizmo_mode;
        bool snap_enabled;
        f32 snap_value;
        bool show_grid;

        char scene_path[256];

        bool rebuild_world;
        bool rebuild_collision;

        i32 ui_active_id;
        i32 ui_hot_id;
        i32 ui_last_mouse_x;
    };

    void editor_init(EditorState* editor);
    void editor_set_active(EditorState* editor, bool active, PlatformState* platform, Player* player);
    void editor_update(EditorState* editor, Scene* scene, PlatformState* platform, f32 dt);
    void editor_draw_ui(EditorState* editor, Scene* scene, PlatformState* platform);
    bool editor_scene_needs_rebuild(const EditorState* editor);
    void editor_clear_rebuild_flag(EditorState* editor);

}

#endif