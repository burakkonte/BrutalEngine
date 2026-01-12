# Brutal Engine

## 1) Project Overview
Brutal Engine is a custom C++ game engine built specifically to power a single gothic FPS horror experience. It focuses on the exact needs of that game—tight first-person movement, moody lighting, and in-engine iteration—rather than competing with general-purpose engines.

## 2) Key Features
**Engine**
- Windows/OpenGL rendering pipeline with forward lighting and debug draw utilities.
- Brush-based world geometry with collision and sweep/slide movement.
- FPS controller with sprint, crouch, coyote time, and jump buffering.
- Built-in debug overlays and lightweight profiling hooks.

**Current Game/Demo**
- Gothic house demo level with a central cross landmark.
- Point/ambient lights tuned for a moody horror atmosphere.
- In-editor scene editing with brushes, props, and lights.

## 3) Repo Structure
```
.
├── brutal/                 # CMake project root
│   ├── engine/             # Engine runtime (public headers + private implementation)
│   ├── playground/         # Demo game (the gothic FPS slice)
│   ├── third_party/        # External deps (e.g., glad)
│   └── out/                # Generated build artifacts (if present)
└── README_BUILD.md         # Original build notes
```

## 4) Build & Run (Windows + Visual Studio 2022 + CMake)
From a Developer PowerShell or standard PowerShell:

```powershell
# 1) Go to the engine source
cd brutal

# 2) Configure
cmake -S . -B build -G "Visual Studio 17 2022"

# 3) Build (Release recommended)
cmake --build build --config Release

# 4) Run the demo
.\build\Release\playground.exe
```

> Tip: Visual Studio users can open the generated solution in `build/` and run the `playground` target (it is set as the startup project on MSVC).

## 5) Controls
**Gameplay**
- **W/A/S/D**: Move
- **Mouse**: Look
- **Space**: Jump
- **Ctrl (hold)**: Crouch
- **Shift (hold)**: Sprint (forward only, while grounded)
- **ESC**: Release mouse capture / quit

**Debug/Overlay Hotkeys**
- **F1**: Main debug overlay
- **F2**: Performance overlay
- **F3**: Render stats
- **F4**: Collision debug
- **F5**: Reload request
- **` (grave)**: Toggle console stub

## 6) Editor Mode / Debug Tools
Editor mode is available in the demo build.

- **Toggle Editor Mode**: **F9**
- **Mouse Capture (Editor)**: Hold **Right Mouse** to freelook; release to unlock.
- **Editor Movement**: **W/A/S/D** move, **Space** up, **Q** down.
- **Selection**: **Left Click** to pick brushes/props/lights.
- **Gizmos**: **W** Translate, **E** Rotate, **R** Scale.
- **Scene Save/Load**: **Ctrl+S** / **Ctrl+O** (uses `scene.json` in the working directory).

Editor UI panels show hierarchy, inspector, asset spawn buttons, and top-bar save/load actions.

## 7) Configuration / Tuning
Movement and feel are tuned in code:

- **Player movement constants**: `brutal/engine/private/world/player.cpp`  
  (walk/sprint/crouch speeds, gravity, jump velocity, mouse sensitivity, coyote time, jump buffer).
- **Editor camera settings**: `brutal/playground/src/editor.cpp`  
  (editor move speed, look sensitivity).
- **Window resolution/title**: `brutal/playground/src/main.cpp`  
  (initial window size and title).

## 8) Roadmap
- Expand the gothic slice into multi-room traversal with streaming/serialization.
- Add basic interaction and horror scripting triggers.
- Improve lighting (shadows, light volumes) and post-processing.
- Asset pipeline improvements for props and materials.

## 9) License + Credits
**License**: TBD  
**Credits**: Uses the `glad` OpenGL loader (see `brutal/third_party`).
