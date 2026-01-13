# Brutal Engine

## Project overview
Brutal Engine is a custom C++ game engine built to power a **single** gothic FPS horror game on Windows. It is not a general-purpose engine and intentionally focuses on the exact runtime/editor needs of that game rather than broad extensibility.

**Scope**
- First-person gothic horror FPS runtime and in-engine editing for the included playground/demo.

**Non-goals**
- No general-purpose engine features, no broad extensibility, and no third-person or multiplayer focus (by design).

## Key features
### Runtime (playground)
- OpenGL renderer with lit/flat shaders, point/spot light support, and debug drawing utilities.
- Brush-based world geometry and world mesh rebuilds from brushes.
- AABB collision world with sweep/slide movement integration for the player controller.
- FPS player controller with jump buffering, coyote time, crouch/sprint, and mouse look support.
- Flashlight system integrated into the lighting environment.
- Debug overlays (FPS, render stats, collision debug, console toggle, reload request).

### Editor (in-game)
- Built-in editor mode with multiple viewports, selection, and transform gizmos.
- Scene save/load to `scene.json` in the working directory (brushes, props, lights).
- Undo/redo stack for editor commands (transform and entity actions).

## Screenshots / GIFs
> Drop your images into this section.

**Runtime**
- `TODO: gameplay_screenshot.png`

**Editor**
- `TODO: editor_screenshot.png`

## Build & run (Windows + CMake + Visual Studio)
**Requirements**
- Windows + Visual Studio 2022 (MSVC)
- CMake 3.20+

**Build (PowerShell)**
```powershell
# From the repository root
cd brutal

cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

**Run the playground**
```powershell
# From brutal/
.\build\Release\playground.exe
```

> Tip: Open the generated solution under `brutal/build/` in Visual Studio and run the `playground` target (it is set as the startup project on MSVC).

## Controls
### Gameplay
- **W/A/S/D**: Move
- **Mouse**: Look (click in the window to capture the mouse)
- **Space**: Jump
- **Ctrl (hold)**: Crouch
- **Shift (hold)**: Sprint (forward + grounded)
- **F**: Toggle flashlight
- **Esc**: Release mouse capture or quit the app

### Debug overlays
- **F1**: Toggle main debug overlay
- **F2**: Toggle performance overlay
- **F3**: Toggle render stats
- **F4**: Toggle collision debug overlay
- **F5**: Request reload (currently logs only)
- **` (grave)**: Toggle console overlay

### Editor mode
- **F9** or **P**: Toggle editor mode
- **Right Mouse (hold)**: Free-look in the perspective viewport
- **W/A/S/D**: Move camera (perspective)
- **Space / Q**: Move up / down (perspective)
- **Left Mouse**: Select; **Shift** adds to selection; drag to box-select
- **W/E/R**: Translate / Rotate / Scale gizmos
- **Q**: Clear gizmo mode
- **G**: Toggle snapping
- **L**: Toggle local/world space for gizmos
- **Ctrl+S / Ctrl+O**: Save / Load `scene.json`
- **Ctrl+Z / Shift+Ctrl+Z**: Undo / Redo
- **Ctrl+Y**: Redo
- **Ctrl+D**: Duplicate selection
- **Delete**: Delete selection
- **F**: Focus active viewport on selection
- **Mouse wheel**: Zoom in orthographic views
- **Middle/Right Mouse (hold)**: Pan in orthographic views

## Project structure
```
BrutalEngine/
├── brutal/                  # CMake project root
│   ├── engine/              # Engine runtime (public + private source)
│   │   ├── public/           # Public headers
│   │   ├── private/          # Implementation
│   │   └── docs/             # Design docs (player controller, debugging)
│   ├── playground/          # Demo game + in-engine editor
│   │   └── src/              # Playground sources
│   ├── third_party/         # External deps (glad)
│   └── out/                 # Build artifacts (if present)
└── README.md
```


## Tech overview (high level)
- **Renderer**: OpenGL-based renderer with lit and flat shaders, point/spot lights, and debug draw helpers for lines/text overlays.
- **Input & platform**: Win32-based platform layer with key/mouse state tracking, mouse capture, and window lifecycle control.
- **Collision & movement**: AABB collision world with move-and-slide, used by the FPS player controller (coyote time + jump buffer logic).
- **Editor system**: In-game editor with multi-viewport setup, picking, gizmos, undo/redo, and JSON save/load for brushes/props/lights.

## Debugging / troubleshooting
- **CMake errors about missing subdirectories**: Ensure the repository was fully extracted and that `brutal/third_party`, `brutal/engine`, and `brutal/playground` exist.
- **No mouse look**: Click inside the game window to capture the mouse, or press **Esc** to release it if captured.
- **Editor feels unresponsive to gameplay input**: Toggle editor mode off (F9 / P) to return to play input.
- **Scene load/save not working**: The editor reads/writes `scene.json` in the working directory. Ensure the app has write permission there.

## Roadmap
Next milestones for the vertical-slice gameplay systems:
- TODO: Expand the gothic demo into multi-room traversal (serialization/streaming as needed).
- TODO: Add basic interaction and scripted horror triggers.
- TODO: Improve lighting/shadows and post-processing.
- TODO: Grow the prop/content pipeline.

## Contributing
- Keep changes aligned with the single-game focus and the player controller design docs in `brutal/engine/docs/`.
- Prefer small, targeted PRs with clear testing notes.
- If you modify controls or data formats, update this README.

## License
License not yet added. (A dedicated `LICENSE` file is required before external redistribution.)
