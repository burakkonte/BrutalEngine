#ifndef PLAYGROUND_ENGINE_MODE_H
#define PLAYGROUND_ENGINE_MODE_H

#include "brutal/core/platform.h"

namespace brutal {

    enum class EngineMode {
        Editor,
        Play,
        DebugFreeCam
    };

    struct EngineModeState {
        EngineMode mode;
        EngineMode previous_mode;
    };

    void engine_mode_init(EngineModeState* state, EngineMode start_mode);
    void engine_mode_update(EngineModeState* state, const InputState* input);
    const char* engine_mode_name(EngineMode mode);

}

#endif
