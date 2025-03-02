#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t* buffer;
    size_t used;
    size_t cap;
} Arena;

Arena arena_new(size_t size);
void arena_free(Arena* arena);
void* arena_alloc(Arena* arena, size_t size);

#endif
