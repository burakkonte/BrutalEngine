# Brutal Engine

Brutal Engine is a C++ game engine and playable prototype built for the **Gothic House** FPS playground. The current codebase focuses on Win32 + OpenGL rendering, brush-based level building, and an in-engine editor UI for the demo scene.

**Scope**
- First-person gothic horror FPS runtime and in-engine editing for the included playground/demo.

**Non-goals (current)**
- Not a general-purpose engine; scope is intentionally limited to the demo’s needs.

## Key Features (Implemented)
### Core systems
- Memory arena allocator, logging, and scoped profiling support with per-frame timing.
- Basic math library (vectors, matrices, quaternions, geometry helpers).

### Platform & input
- Win32 window creation and OpenGL context setup.
- Keyboard/mouse input state tracking with raw mouse deltas and mouse-look telemetry.
- Mouse capture/lock support for FPS input.

### Rendering
- OpenGL renderer using lit + flat shaders (GLSL `#version 330`).
- Point lights and spot lights with ambient lighting controls.
- Debug draw utilities (3D lines, 2D lines, and 2D text overlays).
- Built-in cube and grid mesh helpers.

### World/scene
- Brush-based world geometry with rebuildable world mesh.
- Prop entities (currently rendered as cubes) and point light entities.
- Collision world built from brush AABBs.

### Gameplay
- FPS player controller with walk/sprint/crouch states.
- Jump buffering + coyote time for more reliable jumping.
- AABB sweep/slide collision for player movement.
- Flashlight system implemented as a controllable spot light.

### Editor (in-game)
- ImGui-based docking UI with panels for hierarchy, inspector, and content placeholders.
- Scene viewport rendered to an offscreen framebuffer.
- Transform gizmos via ImGuizmo for brushes, props, and lights.
- Editor camera free-fly controls and optional grid rendering.

### Debugging
- Debug overlays for FPS/timing, renderer stats, and collision visualization.
- Profiler HUD for frame timing breakdown (enabled in non-Release builds).

## Planned / Roadmap
- **Scene serialization** (save/load) for brushes, props, and lights.
- **Undo/redo** stack for editor actions.
- **Editor selection in viewport** (ray picking) to complement the hierarchy list.
- **Console command input** (current console panel is a placeholder).
- **Content browser tooling** beyond the current placeholder panel.
- **Lighting upgrades** (shadows and post-processing).
- **Gameplay expansion** (multi-room traversal, scripted interactions).

## Build & Run
### Prerequisites
- Windows 10/11
- Visual Studio 2022 (MSVC)
- CMake 3.20+
- GPU driver with OpenGL 3.3 support (shaders target GLSL 330)

### Build (PowerShell)
```powershell
# From the repository root
cd brutal

cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
