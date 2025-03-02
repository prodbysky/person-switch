#include "arena.h"
#include "player.h"
#include "stage.h"
#include "static_config.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>

Stage default_stage(Arena *arena);

int main() {
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    SetTargetFPS(120);
    Arena stage_allocator = arena_new(1024 * 4);
    Stage st = default_stage(&stage_allocator);
    Player player = player_new();
    while (!WindowShouldClose()) {
        player_update(&player, &st);
        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
        DrawRectangleRec(player.rect, WHITE);
        draw_stage(&st);
        EndDrawing();
    }
    CloseWindow();
}

Stage default_stage(Arena *arena) {
    Stage st;
    st.count = 2;
    st.platforms = arena_alloc(arena, sizeof(Platform) * 3);
    st.platforms[0] = (Platform){
        .x = 100,
        .y = 500,
        .width = 128,
        .height = 32,
    };
    st.platforms[1] = (Platform){
        .x = 600,
        .y = 500,
        .width = 128,
        .height = 32,
    };

    return st;
}
