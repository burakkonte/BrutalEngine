#include "engine_mode.h"

namespace brutal {

    void engine_mode_init(EngineModeState* state, EngineMode start_mode) {
        if (!state) return;
        state->mode = start_mode;
        state->previous_mode = start_mode;
    }

    const char* engine_mode_name(EngineMode mode) {
        switch (mode) {
        case EngineMode::Editor: return "Editor";
        case EngineMode::Play: return "Play";
        case EngineMode::DebugFreeCam: return "Debug FreeCam";
        default: return "Unknown";
        }
    }

    void engine_mode_update(EngineModeState* state, const InputState* input) {
        if (!state || !input) return;
        if (platform_key_pressed_raw(input, KEY_F9)) {
            if (state->mode == EngineMode::Editor) {
                state->mode = EngineMode::Play;
            }
            else if (state->mode == EngineMode::Play) {
                state->mode = EngineMode::Editor;
            }
            else {
                state->previous_mode = (state->previous_mode == EngineMode::Editor)
                    ? EngineMode::Play
                    : EngineMode::Editor;
            }
        }

        if (platform_key_pressed_raw(input, KEY_F10)) {
            if (state->mode == EngineMode::DebugFreeCam) {
                state->mode = state->previous_mode;
            }
            else {
                state->previous_mode = state->mode;
                state->mode = EngineMode::DebugFreeCam;
            }
        }
    }

}
