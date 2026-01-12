#include "brutal/core/memory.h"
#include "brutal/core/logging.h"
#include <cstdlib>
#include <cstring>

namespace brutal {

bool memory_init(MemoryState* state, size_t persistent_size, size_t frame_size) {
    state->persistent.base = static_cast<u8*>(malloc(persistent_size));
    if (!state->persistent.base) return false;
    state->persistent.size = persistent_size;
    state->persistent.used = 0;
    
    state->frame.base = static_cast<u8*>(malloc(frame_size));
    if (!state->frame.base) {
        free(state->persistent.base);
        return false;
    }
    state->frame.size = frame_size;
    state->frame.used = 0;
    
    LOG_INFO("Memory initialized: persistent=%zuMB, frame=%zuMB", 
             persistent_size / (1024*1024), frame_size / (1024*1024));
    return true;
}

void memory_shutdown(MemoryState* state) {
    free(state->persistent.base);
    free(state->frame.base);
    state->persistent = {};
    state->frame = {};
}

bool arena_init(MemoryArena* arena, size_t size) {
    arena->base = static_cast<u8*>(malloc(size));
    if (!arena->base) return false;
    arena->size = size;
    arena->used = 0;
    return true;
}

void arena_shutdown(MemoryArena* arena) {
    free(arena->base);
    arena->base = nullptr;
    arena->size = 0;
    arena->used = 0;
}

void arena_reset(MemoryArena* arena) {
    arena->used = 0;
}

void* arena_alloc(MemoryArena* arena, size_t size, size_t align) {
    size_t aligned = (arena->used + align - 1) & ~(align - 1);
    if (aligned + size > arena->size) {
        LOG_ERROR("Arena out of memory");
        return nullptr;
    }
    void* ptr = arena->base + aligned;
    arena->used = aligned + size;
    memset(ptr, 0, size);
    return ptr;
}

}
