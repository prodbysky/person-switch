#include "arena.h"
#include "enemy.h"
#include "player.h"
#include "stage.h"
#include "static_config.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>

Stage default_stage(Arena *arena);

typedef struct {
    Arena allocator;
    ECSPlayer player;
    Stage stage;
} GameState;

GameState game_state_init() {
    Arena global_game_allocator = arena_new(1024 * 4);
    Stage st = default_stage(&global_game_allocator);
    ECSPlayer player = ecs_player_new();

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
    ECSEnemy test = ecs_enemy_new((Vector2){100, 100}, (Vector2){64, 64}, 20);

    while (!WindowShouldClose()) {
        double before_update = GetTime() * 10000;
        ecs_player_update(&state.player, &state.stage);
        ecs_enemy_update(&test, &state.stage, &state.player.collider);
        double after_update = GetTime() * 10000;
        BeginDrawing();
        DrawFPS(10, 10);
        DrawText(TextFormat("Update time: %.2f (ms * 10).", after_update - before_update), 10, 40, 20, WHITE);
        DrawText(TextFormat("Heap usage: %u bytes used", state.allocator.used), 10, 70, 20, WHITE);

        ClearBackground(GetColor(0x181818ff));
        DrawRectangleRec(state.player.collider.rect, WHITE);
        DrawRectangleRec(test.collider.rect, BLUE);
        draw_stage(&state.stage);
        EndDrawing();
    }
    CloseWindow();
    game_state_destroy(&state);
}

Stage default_stage(Arena *arena) {
    Stage st;
    st.count = 3;
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
    st.platforms[2] = (Platform){
        .x = 0,
        .y = 550,
        .width = 800,
        .height = 32,
    };

    return st;
}
