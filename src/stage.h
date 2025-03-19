#ifndef STAGE_H
#define STAGE_H
#include <raylib.h>
#include <stddef.h>
typedef Rectangle Platform;

typedef struct {
    Vector2 spawn;
    Platform platforms[50];
    Platform spawns[20];
    size_t count;
    size_t count_sp;
} Stage;

void draw_stage(const Stage *stage);
#endif
