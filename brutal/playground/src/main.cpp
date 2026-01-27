// =============================================================================
// Brutal Engine - Playground Demo
// Gothic House with Cross Landmark
// =============================================================================

#include "brutal/brutal.h"
#include "brutal/core/platform.h"
#include "brutal/core/logging.h"
#include "brutal/core/memory.h"
#include "brutal/core/time.h"
#include "brutal/renderer/gl_context.h"
#include "brutal/renderer/renderer.h"
#include "brutal/renderer/debug_draw.h"
#include "brutal/world/scene.h"
#include "brutal/world/player.h"
#include "debug_system.h"
#include "editor/Editor.h"
#include <glad/glad.h>
#include <cstdio>

using namespace brutal;

// =============================================================================
// Level Building - Gothic House with Cross
// =============================================================================

static void build_gothic_house(Scene* scene) {
    // Dark stone colors for gothic atmosphere
    Vec3 dark_stone(0.15f, 0.12f, 0.10f);
    Vec3 floor_color(0.08f, 0.06f, 0.05f);
    Vec3 ceiling_color(0.05f, 0.04f, 0.03f);
    Vec3 wood_color(0.25f, 0.15f, 0.08f);
    
    // Room dimensions
    const f32 room_size = 12.0f;
    const f32 room_height = 4.0f;
    const f32 wall_thick = 0.3f;
    
    // Floor
    scene_add_brush(scene, 
        Vec3(-room_size, -wall_thick, -room_size),
        Vec3(room_size, 0, room_size),
        BRUSH_SOLID, floor_color);
    
    // Ceiling
    scene_add_brush(scene,
        Vec3(-room_size, room_height, -room_size),
        Vec3(room_size, room_height + wall_thick, room_size),
        BRUSH_SOLID, ceiling_color);
    
    // Walls - North
    scene_add_brush(scene,
        Vec3(-room_size, 0, -room_size - wall_thick),
        Vec3(room_size, room_height, -room_size),
        BRUSH_SOLID, dark_stone);
    
    // Walls - South
    scene_add_brush(scene,
        Vec3(-room_size, 0, room_size),
        Vec3(room_size, room_height, room_size + wall_thick),
        BRUSH_SOLID, dark_stone);
    
    // Walls - East
    scene_add_brush(scene,
        Vec3(room_size, 0, -room_size),
        Vec3(room_size + wall_thick, room_height, room_size),
        BRUSH_SOLID, dark_stone);
    
    // Walls - West
    scene_add_brush(scene,
        Vec3(-room_size - wall_thick, 0, -room_size),
        Vec3(-room_size, room_height, room_size),
        BRUSH_SOLID, dark_stone);
    
    // =========================================================================
    // CROSS LANDMARK - Center of Room
    // =========================================================================
    Vec3 cross_color(0.6f, 0.55f, 0.45f);  // Pale stone/bone color
    f32 cx = 0.0f, cz = 0.0f;  // Center position
    f32 cross_base = 0.0f;      // Floor level
    
    // Cross base/pedestal
    scene_add_brush(scene,
        Vec3(cx - 0.5f, cross_base, cz - 0.5f),
        Vec3(cx + 0.5f, cross_base + 0.2f, cz + 0.5f),
        BRUSH_SOLID, Vec3(0.2f, 0.15f, 0.12f));
    
    // Vertical beam of cross
    f32 beam_width = 0.15f;
    f32 cross_height = 2.5f;
    scene_add_brush(scene,
        Vec3(cx - beam_width, cross_base + 0.2f, cz - beam_width),
        Vec3(cx + beam_width, cross_base + cross_height, cz + beam_width),
        BRUSH_SOLID, cross_color);
    
    // Horizontal beam of cross (crossbar)
    f32 arm_length = 0.8f;
    f32 arm_height = 1.8f;  // Height where crossbar sits
    scene_add_brush(scene,
        Vec3(cx - arm_length, cross_base + arm_height, cz - beam_width),
        Vec3(cx + arm_length, cross_base + arm_height + beam_width * 2, cz + beam_width),
        BRUSH_SOLID, cross_color);
    
    // Decorative pillars in corners
    f32 pillar_size = 0.4f;
    f32 pillar_height = 3.5f;
    Vec3 pillar_color(0.12f, 0.10f, 0.08f);
    
    // Corner pillars
    f32 corner_offset = room_size - 1.0f;
    scene_add_brush(scene, Vec3(-corner_offset, 0, -corner_offset), 
                    Vec3(-corner_offset + pillar_size, pillar_height, -corner_offset + pillar_size), 
                    BRUSH_SOLID, pillar_color);
    scene_add_brush(scene, Vec3(corner_offset - pillar_size, 0, -corner_offset), 
                    Vec3(corner_offset, pillar_height, -corner_offset + pillar_size), 
                    BRUSH_SOLID, pillar_color);
    scene_add_brush(scene, Vec3(-corner_offset, 0, corner_offset - pillar_size), 
                    Vec3(-corner_offset + pillar_size, pillar_height, corner_offset), 
                    BRUSH_SOLID, pillar_color);
    scene_add_brush(scene, Vec3(corner_offset - pillar_size, 0, corner_offset - pillar_size), 
                    Vec3(corner_offset, pillar_height, corner_offset), 
                    BRUSH_SOLID, pillar_color);
    
    // =========================================================================
    // Lighting - Gothic/Moody Atmosphere
    // =========================================================================
    
    // Very dim cool ambient (moonlight seeping in)
    scene->lights.ambient_color = Vec3(0.05f, 0.05f, 0.08f);
    scene->lights.ambient_intensity = 0.3f;
    
    // Warm candle light near the cross - main focus
    light_environment_add_point(&scene->lights,
        Vec3(0.0f, 1.5f, 0.5f),      // Near cross, at eye level
        Vec3(1.0f, 0.7f, 0.4f),      // Warm orange-yellow
        5.0f,                         // Medium radius
        2.0f);                        // Moderate intensity
    
    // Secondary warm light behind cross (backlight glow)
    light_environment_add_point(&scene->lights,
        Vec3(0.0f, 2.5f, -1.0f),     // Behind and above cross
        Vec3(1.0f, 0.6f, 0.3f),      // Orange
        4.0f,
        1.5f);
    
    // Dim blue accent lights in corners (moonlight)
    light_environment_add_point(&scene->lights,
        Vec3(-corner_offset + 0.5f, 2.0f, -corner_offset + 0.5f),
        Vec3(0.4f, 0.5f, 0.8f),      // Cool blue
        3.0f,
        0.5f);
    
    light_environment_add_point(&scene->lights,
        Vec3(corner_offset - 0.5f, 2.0f, corner_offset - 0.5f),
        Vec3(0.4f, 0.5f, 0.8f),
        3.0f,
        0.5f);
    
    LOG_INFO("Gothic house with cross landmark built");
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    LOG_INFO("Brutal Engine - Gothic House Demo");
    LOG_INFO("Controls: WASD move, SPACE jump, CTRL crouch, SHIFT sprint, ESC quit, F1-F4 debug");
    
    // Initialize platform
    PlatformState platform = {};
    if (!platform_init(&platform, "Brutal Engine - Gothic House", 1280, 720)) {
        LOG_ERROR("Failed to initialize platform");
        return 1;
    }
    
    // Initialize OpenGL
    if (!gl_init()) {
        LOG_ERROR("Failed to initialize OpenGL");
        platform_shutdown(&platform);
        return 1;
    }
    
    // Memory arena for allocations
    MemoryArena arena = {};
    arena_init(&arena, 64 * 1024 * 1024);  // 64 MB
    
    MemoryArena temp_arena = {};
    arena_init(&temp_arena, 16 * 1024 * 1024);  // 16 MB temp
    
    // Initialize renderer
    RendererState renderer = {};
    if (!renderer_init(&renderer, &arena)) {
        LOG_ERROR("Failed to initialize renderer");
        platform_shutdown(&platform);
        return 1;
    }
    
    // Initialize debug drawing
    if (!debug_draw_init()) {
        LOG_ERROR("Failed to initialize debug drawing");
        return 1;
    }

    profiler_init();
    
    // Create scene
    Scene scene = {};
    if (!scene_create(&scene, &arena)) {
        LOG_ERROR("Failed to create scene");
        return 1;
    }
    
    // Build the gothic house level
    build_gothic_house(&scene);
    
    // Rebuild world mesh and collision
    scene_rebuild_world_mesh(&scene, &temp_arena);
    scene_rebuild_collision(&scene);
    
    // Initialize player
    Player player = {};
    player_init(&player);
    player.camera.position = Vec3(0, 1.7f, 8);  // Start facing the cross
    player.camera.yaw = 3.14159f;                // Face towards -Z (towards cross)
    
    // Timing
    f64 last_time = time_now();
    f64 accumulator = 0.0;
    const f64 fixed_dt = 1.0 / 60.0;
    
    DebugSystem debug_system = {};
    debug_system_init(&debug_system);

    EditorContext editor = {};
    editor_init(&editor, &platform);
    
    // Main loop
    while (!platform.should_quit) {
        // Calculate delta time
        f64 current_time = time_now();
        f64 frame_dt = current_time - last_time;
        last_time = current_time;
        
        // Clamp delta time to prevent spiral of death
        if (frame_dt > 0.25) frame_dt = 0.25;
        
        // Poll input
        platform_poll_events(&platform);

        profiler_begin_frame();
        
        // Handle escape key
        if (platform_key_pressed(&platform.input, KEY_ESCAPE)) {
            if (platform.mouse_captured) {
                platform_disable_mouse_look(&platform);
            } else {
                platform.should_quit = true;
            }
        }

        if (platform_key_pressed(&platform.input, KEY_F9) || platform_key_pressed(&platform.input, KEY_P)) {
            bool next_active = !editor.active;
            editor_set_active(&editor, next_active, &platform, &player);
            
        }

        player_capture_input(&player, &platform.input, editor.active);
        
        debug_system_update(&debug_system, &platform.input);
        if (debug_system_consume_reload(&debug_system)) {
            LOG_INFO("Reload requested (not implemented)");
        }
        
        // Capture mouse on click (play mode)
        if (!editor.active && platform.input.mouse.left.pressed && !platform.mouse_look_enabled) {
            platform_enable_mouse_look(&platform);
        }

        if (editor.active && platform.mouse_look_enabled) {
            platform_disable_mouse_look(&platform);
        }

        MouseDelta look_delta = platform_consume_mouse_delta(&platform);
        const bool ui_mouse_capture = editor.active || !platform.mouse_look_enabled || !platform.input_focused;
        PlayerLookResult look_result = player_apply_mouse_look(&player, &platform.input, ui_mouse_capture);
        platform_mouse_look_record(&platform,
            (f32)frame_dt,
            (f32)(frame_dt * 1000.0),
            platform.input.mouse.raw_dx,
            platform.input.mouse.raw_dy,
            look_delta.dx,
            look_delta.dy,
            look_result.yaw_delta,
            look_result.pitch_delta,
            editor.active);
        
        // Fixed timestep physics update
        accumulator += frame_dt;
        int fixed_steps = 0;
        while (accumulator >= fixed_dt) {
            fixed_steps++;
            player_set_frame_info(&player, (f32)frame_dt, fixed_steps, fixed_steps);
            if (!editor.active && platform.mouse_captured) {
                PROFILE_SCOPE("Player Update");
                player_update(&player, &platform.input, &scene.collision, (f32)fixed_dt);
            }
            accumulator -= fixed_dt;
        }

        player_set_frame_info(&player, (f32)frame_dt, fixed_steps, fixed_steps);

        if (editor.active) {
            editor_begin_frame(&editor, &platform);
            editor_build_ui(&editor, &scene, &platform);
            editor_update(&editor, &scene, &platform, (f32)frame_dt);
        }

        player_update_flashlight(&player, (f32)frame_dt);
        player_apply_flashlight(&player, &player.camera, &scene.lights, !editor.active);
        
        // Clear temp arena each frame
        arena_reset(&temp_arena);
        if (editor.active && editor_scene_needs_rebuild(&editor)) {
            scene_rebuild_world_mesh(&scene, &temp_arena);
            if (editor.rebuild_collision) {
                scene_rebuild_collision(&scene);
            }
            editor_clear_rebuild_flag(&editor);
        }
        
        // Render
        PROFILE_SCOPE("Render");
        renderer_begin_frame(&renderer, platform.window_width, platform.window_height);

        renderer_set_lights(&renderer, &scene.lights);
        
        if (editor.active) {
            editor_render_scene(&editor, &scene, &renderer);
        }
        else {
            renderer_set_camera(&renderer, &player.camera);
            if (scene.world_mesh.vao) {
                renderer_draw_mesh(&renderer, &scene.world_mesh, Mat4::identity(), Vec3(1, 1, 1));
            }

            for (u32 i = 0; i < scene.prop_count; ++i) {
                const PropEntity& prop = scene.props[i];
                if (!prop.active) continue;
                Mat4 model = transform_to_matrix(&prop.transform);
                renderer_draw_mesh(&renderer, renderer_get_cube_mesh(&renderer), model, prop.color);
            }
        }
        
        DebugFrameInfo frame_info = {};
        frame_info.delta_time = (f32)frame_dt;
        frame_info.frame_ms = (f32)(frame_dt * 1000.0);
        frame_info.fps = (frame_dt > 0.0) ? (f32)(1.0 / frame_dt) : 0.0f;
        debug_system_draw(&debug_system, frame_info, &platform.input, &platform, &player, &renderer, &scene.collision,
            platform.window_width, platform.window_height);
        
        if (debug_system_show_collision(&debug_system)) {
            if (editor.active) {
                debug_lines_flush(&editor.camera, platform.window_width, platform.window_height);
            }
            else {
                debug_lines_flush(&player.camera, platform.window_width, platform.window_height);
            }
        }

        debug_lines_flush_2d(platform.window_width, platform.window_height);

        debug_text_flush(platform.window_width, platform.window_height);

        profiler_end_frame();

        if (editor.active) {
            editor_end_frame(&editor, &platform);
        }
        
        renderer_end_frame();
        platform_swap_buffers(&platform);
    }
    
    // Cleanup
    profiler_shutdown();
    debug_draw_shutdown();
    editor_shutdown(&editor);
    scene_destroy(&scene);
    renderer_shutdown(&renderer);
    arena_shutdown(&arena);
    arena_shutdown(&temp_arena);
    platform_shutdown(&platform);
    
    LOG_INFO("Shutdown complete");
    return 0;
}
