#ifndef BRUTAL_WORLD_SCENE_IO_H
#define BRUTAL_WORLD_SCENE_IO_H

#include "brutal/core/types.h"
#include "brutal/world/scene.h"

namespace brutal {

    struct SceneSpawn {
        Vec3 position;
        f32 yaw;
        f32 pitch;
    };

    bool scene_load_from_json(Scene* scene, SceneSpawn* spawn, const char* path, MemoryArena* arena);

}

#endif
