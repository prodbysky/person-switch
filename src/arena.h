#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

// Really simple bone basic arena allocator that crashes
// your program if the arena overflew
typedef struct {
    uint8_t* buffer;
    size_t used;
    size_t cap;
} Arena;

// Create an arena with the capacity of `size`
Arena arena_new(size_t size);
void arena_free(Arena* arena);
void* arena_alloc(Arena* arena, size_t size);

#endif
