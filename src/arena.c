#include "arena.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Arena arena_new(size_t size) {
    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * size);
    assert(buffer && "Failed to allocate memory for an arena: Buy more ram!!!");
    return (Arena){.buffer = buffer, .used = 0, .cap = size};
}

void arena_free(Arena *arena) {
    free(arena->buffer);
    arena->buffer = 0;
    arena->cap = 0;
    arena->buffer = 0;
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t alligned = (size + 7) & ~7;

    if (arena->used + alligned >= arena->cap) {
        assert(false && "Arena overflow!: Buy more ram!!!");
    }
    uint8_t *buf = &arena->buffer[arena->used];
    arena->used += alligned;
    return (void *)buf;
}
