#ifndef STAGE_H
#define STAGE_H
#include <raylib.h>
#include <stddef.h>
typedef Rectangle Platform;

typedef struct {
    Platform platforms[50];
    size_t count;
} Stage;

void draw_stage(const Stage *stage);
#endif
