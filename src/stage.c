#include "stage.h"

void draw_stage(const Stage *stage) {
    for (size_t i = 0; i < stage->count; i++) {
        DrawRectangleRec(stage->platforms[i], RED);
    }
}
