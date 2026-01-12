# Brutal Engine - Build Instructions

## Requirements
- Windows 10/11
- Visual Studio 2022 (or Visual Studio Build Tools with MSVC)
- CMake 3.20+

## Build Steps

```powershell
# Extract the archive
unzip brutal_engine.zip
cd brutal

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022"

# Build
cmake --build . --config Release

# Run the demo
.\Release\playground.exe
```

## Controls
| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look around |
| Space | Jump |
| Ctrl | Hold to crouch |
| Shift | Sprint (forward only) |
| F1 | Toggle debug overlay |
| ESC | Release mouse / Quit |

## Features Implemented
- Full FPS physics (gravity, jump, crouch, sprint)
- Coyote time (0.1s grace period after leaving ground)
- Jump buffering (0.1s input buffer before landing)
- Sweep-and-slide collision detection
- Forward renderer with point lights
- Debug text overlay with 8x8 bitmap font
- Gothic house demo level with cross landmark

## Project Structure
```
brutal/
├── engine/
│   ├── public/brutal/     # Headers
│   └── private/           # Implementation
├── playground/src/        # Demo application
└── third_party/glad/      # OpenGL loader
```

## Physics Constants (tunable in player.cpp)
- Gravity: 20 m/s²
- Jump velocity: 6.5 m/s
- Walk speed: 4.5 m/s
- Sprint speed: 7.5 m/s
- Crouch speed: 2.5 m/s
- Air control: 30%
