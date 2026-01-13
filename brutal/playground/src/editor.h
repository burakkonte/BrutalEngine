#ifndef PLAYGROUND_EDITOR_H
#define PLAYGROUND_EDITOR_H

#include "brutal/core/types.h"
#include "brutal/renderer/camera.h"
#include "brutal/world/scene.h"

#include <vector>

namespace brutal {

    struct PlatformState;
    struct Player;
    struct RendererState;
    struct Rect {
        i32 x;
        i32 y;
        i32 w;
        i32 h;
    };

    enum class ViewportType {
        Perspective
    };

    struct Viewport {
        i32 id;
        Rect rect;
        Camera camera;
        ViewportType type;
        bool isHovered;
        bool isActive;
    };
    enum class SelectionType {
        None,
        Brush,
        Prop,
        Light
    };

    struct TransformCommand {
        SelectionType type;
        u32 index;
        Transform before;
        Transform after;
    };
    struct EditorState {
        bool active;
        Camera camera;
        f32 move_speed;
        f32 look_sensitivity;

        

        SelectionType selection_type;
        u32 selection_index;
        i32 selectedEntityId;

        enum class GizmoMode {
            None,
            Translate,
            Rotate,
            Scale
        };

        GizmoMode gizmo_mode;
        bool snap_enabled;
        f32 snap_value;
        bool rotate_snap_enabled;
        f32 rotate_snap_value;
        bool scale_snap_enabled;
        f32 scale_snap_value;
        bool show_grid;

        enum class GizmoAxis {
            None,
            X,
            Y,
            Z
        };

        GizmoAxis gizmo_axis_hot;
        GizmoAxis gizmo_axis_active;
        bool gizmo_drag_active;
        Vec3 gizmo_drag_start_pos;
        Vec3 gizmo_drag_plane_normal;
        Vec3 gizmo_drag_plane_point;
        Vec3 gizmo_drag_start_hit;
        Vec3 gizmo_drag_axis;
        Transform gizmo_drag_start_transform;
        i32 gizmo_drag_entity_id;
        SelectionType gizmo_drag_type;
        u32 gizmo_drag_index;
        bool gizmo_local_space;

        std::vector<TransformCommand> undo_stack;
        std::vector<TransformCommand> redo_stack;
        bool inspector_edit_active;
        i32 inspector_edit_ui_id;
        SelectionType inspector_edit_type;
        u32 inspector_edit_index;
        Transform inspector_edit_before;

        char scene_path[256];

        bool rebuild_world;
        bool rebuild_collision;
        i32 activeViewportId;

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