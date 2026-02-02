#include "brutal/world/scene_io.h"
#include "brutal/core/logging.h"

#include <cstdio>

namespace brutal {

    bool scene_load_from_json(Scene* scene, SceneSpawn* spawn, const char* path, MemoryArena* arena) {
        (void)spawn;
        (void)arena;

        if (!scene) {
            LOG_ERROR("Scene load failed: scene is null");
            return false;
        }

        scene_clear(scene);

        if (!path || path[0] == '\0') {
            LOG_WARN("Scene load skipped: no path provided");
            return true;
        }

        FILE* file = std::fopen(path, "rb");
        if (!file) {
            LOG_WARN("Scene JSON not found: %s (loading skipped)", path);
            return true;
        }

        std::fclose(file);
        LOG_WARN("Scene JSON loading not implemented yet: %s", path);
        return true;
    }

}
