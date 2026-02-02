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
#include "brutal/world/scene_io.h"
#include "brutal/world/player.h"
#include "debug_system.h"
#include "debug_camera.h"
#include "engine_mode.h"
#include "editor/Editor.h"
#include <glad/glad.h>
#include <cstdio>

using namespace brutal;



// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    LOG_INFO("Brutal Engine - Gothic House Demo");
    LOG_INFO("Controls: WASD move, SPACE jump, CTRL crouch, SHIFT sprint, ESC quit");
    LOG_INFO("Modes: F9 toggle Editor/Play, F10 toggle Debug FreeCam");
    LOG_INFO("Debug: F1 main, F2 perf, F3 render, F4 collision, F5 lights, F6 player bounds, F7 reload");
    
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
    
    // Load scene data (data-driven, no hardcoded level)
    const char* scene_path = "playground/data/gothic_house.scene.json";
    SceneSpawn spawn = { Vec3(0.0f, 1.7f, 8.0f), 3.14159f, 0.0f };
    if (!scene_load_from_json(&scene, &spawn, scene_path, &arena)) {
        LOG_ERROR("Failed to load scene: %s", scene_path);
        return 1;
    }
    
    // Rebuild world mesh and collision
    scene_rebuild_world_mesh(&scene, &temp_arena);
    scene_rebuild_collision(&scene);
    
    // Initialize player
    Player player = {};
    player_init(&player);
    player.camera.position = spawn.position;
    player.camera.yaw = spawn.yaw;
    player.camera.pitch = spawn.pitch;
    
    // Timing
    f64 last_time = time_now();
    f64 accumulator = 0.0;
    const f64 fixed_dt = 1.0 / 60.0;
    
    DebugSystem debug_system = {};
    debug_system_init(&debug_system);

    EditorContext editor = {};
    editor_init(&editor, &platform);

    EngineModeState engine_mode = {};
    engine_mode_init(&engine_mode, EngineMode::Editor);
    editor_set_active(&editor, true, &platform, &player);

    DebugFreeCamera debug_camera = {};
    debug_free_camera_init(&debug_camera);
    
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
        
        EngineMode previous_mode = engine_mode.mode;
        engine_mode_update(&engine_mode, &platform.input);
        if (engine_mode.mode != previous_mode) {
            if (engine_mode.mode == EngineMode::Editor) {
                editor_set_active(&editor, true, &platform, &player);
                platform_disable_mouse_look(&platform);
            } else {
                editor_set_active(&editor, false, &platform, &player);
                platform_disable_mouse_look(&platform);
            }
        }

        
        
        debug_system_update(&debug_system, &platform.input);
        if (debug_system_consume_reload(&debug_system)) {
            LOG_INFO("Reload requested (not implemented)");
        }
        
        if (engine_mode.mode == EngineMode::Play) {
            // Capture mouse on click (play mode)
            if (!platform.input.mouse_consumed &&
                platform.input.mouse.left.pressed && !platform.mouse_look_enabled) {
                platform_enable_mouse_look(&platform);
            }
        }
        else if (engine_mode.mode == EngineMode::DebugFreeCam) {
            if (platform.input.mouse.right.down && !platform.mouse_look_enabled) {
                platform_enable_mouse_look(&platform);
            }
            else if (!platform.input.mouse.right.down && platform.mouse_look_enabled) {
                platform_disable_mouse_look(&platform);
            }
        }
        else if (engine_mode.mode == EngineMode::Editor && platform.mouse_look_enabled) {
            platform_disable_mouse_look(&platform);
        }

        MouseDelta look_delta = platform_consume_mouse_delta(&platform);
        if (engine_mode.mode == EngineMode::Play) {
            const bool ui_mouse_capture = !platform.mouse_look_enabled || !platform.input_focused;
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
        }

        if (engine_mode.mode == EngineMode::Play) {
            player_capture_input(&player, &platform.input, false);
        }
        
        // Fixed timestep physics update
        accumulator += frame_dt;
        int fixed_steps = 0;
        while (accumulator >= fixed_dt) {
            fixed_steps++;
            player_set_frame_info(&player, (f32)frame_dt, fixed_steps, fixed_steps);
            if (engine_mode.mode == EngineMode::Play && platform.mouse_captured) {
                PROFILE_SCOPE("Player Update");
                player_update(&player, &platform.input, &scene.collision, (f32)fixed_dt);
            }
            accumulator -= fixed_dt;
        }

        player_set_frame_info(&player, (f32)frame_dt, fixed_steps, fixed_steps);

        if (engine_mode.mode == EngineMode::Editor) {
            editor_begin_frame(&editor, &platform);
            editor_build_ui(&editor, &scene, &platform);
            editor_update(&editor, &scene, &platform, (f32)frame_dt);
            if (editor.wants_capture_keyboard) {
                platform_input_consume_keyboard(&platform.input);
            }
            if (editor.wants_capture_mouse) {
                platform_input_consume_mouse(&platform.input);
            }
            player_apply_flashlight(&player, &player.camera, &scene.lights, false);
        }
        else if (engine_mode.mode == EngineMode::DebugFreeCam) {
            debug_free_camera_update(&debug_camera, &platform.input, (f32)frame_dt);
            player_apply_flashlight(&player, &player.camera, &scene.lights, false);
        }
        else if (engine_mode.mode == EngineMode::Play) {
            player_update_flashlight(&player, (f32)frame_dt);
            player_apply_flashlight(&player, &player.camera, &scene.lights, true);
        }

        
        
        // Clear temp arena each frame
        arena_reset(&temp_arena);
        if (engine_mode.mode == EngineMode::Editor && editor_scene_needs_rebuild(&editor)) {
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
        
        const Camera* active_camera = nullptr;
        if (engine_mode.mode == EngineMode::Editor) {
            editor_render_scene(&editor, &scene, &renderer);
            active_camera = &editor.camera;
        }
        else {
            active_camera = (engine_mode.mode == EngineMode::DebugFreeCam)
                ? &debug_camera.camera
                : &player.camera;
            renderer_set_camera(&renderer, active_camera);
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
        debug_system_draw(&debug_system, frame_info, &platform.input, &platform, &player, &renderer, &scene,
            &scene.collision,
            platform.window_width, platform.window_height);
        
        if (debug_system_has_world_lines(&debug_system)) {
            debug_lines_flush(active_camera, platform.window_width, platform.window_height);
        }

        debug_lines_flush_2d(platform.window_width, platform.window_height);

        debug_text_flush(platform.window_width, platform.window_height);

        profiler_end_frame();

        if (engine_mode.mode == EngineMode::Editor) {
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
