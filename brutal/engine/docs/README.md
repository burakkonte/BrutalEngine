# Docs Index (Brutal Engine - Player/Movement)

## Purpose
These documents are the production source of truth for the horror FPS player controller, with a focus on deterministic input handling, sweep/slide movement, and fixing intermittent jump (SPACE) misses. Read them before coding, and keep implementation aligned with the rules, update order, and debug hooks described here.

## How to Use
1. **Start with the main design doc** to implement the player split and deterministic pipeline.
2. **Use the debug doc** when verifying the jump pipeline or chasing intermittent misses.
3. **Keep these docs updated** if you change any part of input, timers, or movement order.

## Documents
- **fps_player_controller.md** — Main design spec for player controller architecture, data, update order, and jump reliability.
- **debug_input_jump.md** — Focused bug-hunt guide for diagnosing intermittent SPACE/jump failures.
