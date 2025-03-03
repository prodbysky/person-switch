#include "arena.h"
#include "player.h"
#include "stage.h"
#include "static_config.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>

Stage default_stage(Arena *arena);

typedef struct {
    Arena allocator;
    Player player;
    Stage stage;
} GameState;

GameState game_state_init() {
    Arena global_game_allocator = arena_new(1024 * 4);
    Stage st = default_stage(&global_game_allocator);
    Player player = player_new();

    return (GameState){
        .allocator = global_game_allocator,
        .player = player,
        .stage = st,
    };
}

void game_state_destroy(GameState *state) {
    arena_free(&state->allocator);
}

int main() {
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    SetTargetFPS(120);

    GameState state = game_state_init();

    while (!WindowShouldClose()) {
        player_update(&state.player, &state.stage);
        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
        DrawRectangleRec(state.player.rect, WHITE);
        draw_stage(&state.stage);
        EndDrawing();
    }
    CloseWindow();
    game_state_destroy(&state);
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
