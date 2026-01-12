#ifndef BRUTAL_CORE_MEMORY_H
#define BRUTAL_CORE_MEMORY_H

#include "brutal/core/types.h"
#include <cstddef>

namespace brutal {

struct MemoryArena {
    u8* base;
    size_t size;
    size_t used;
};

struct MemoryState {
    MemoryArena persistent;
    MemoryArena frame;
};

bool memory_init(MemoryState* state, size_t persistent_size, size_t frame_size);
void memory_shutdown(MemoryState* state);

// Individual arena functions
bool arena_init(MemoryArena* arena, size_t size);
void arena_shutdown(MemoryArena* arena);
void arena_reset(MemoryArena* arena);
void* arena_alloc(MemoryArena* arena, size_t size, size_t align = 8);

template<typename T>
T* arena_alloc_array(MemoryArena* arena, size_t count) {
    return static_cast<T*>(arena_alloc(arena, sizeof(T) * count, alignof(T)));
}

}

#endif
