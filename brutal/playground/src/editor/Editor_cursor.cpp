#include "editor/Editor_cursor.h"

#include "brutal/core/platform.h"

namespace brutal {

    void editor_cursor_set_editor_mode(PlatformState* platform) {
        if (!platform) return;
        platform_disable_mouse_look(platform);
    }

    void editor_cursor_set_game_mode(PlatformState* platform) {
        if (!platform) return;
        platform_disable_mouse_look(platform);
    }

}
