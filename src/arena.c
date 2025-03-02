#include "arena.h"
#include <raylib.h>
#include <stdint.h>
#include <string.h>

Arena arena_new(size_t size) {
    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * size);
    assert(buffer && "Failed to allocate memory for an arena: Buy more ram!!!");
    return (Arena){.buffer = buffer, .used = 0, .cap = size};
}

void arena_free(Arena *arena) {
    free(arena->buffer);
    memset(arena, 0, sizeof(Arena));
}

void *arena_alloc(Arena *arena, size_t size) {
    if (arena->used + size >= arena->cap) {
        assert(false && "Arena overflow!: Buy more ram!!!");
    }
    uint8_t *buf = &arena->buffer[arena->used];
    arena->used += size;
    return (void *)buf;
}
