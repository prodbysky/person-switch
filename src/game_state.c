#include "game_state.h"
#include "enemy.h"
#include "player.h"
#include "static_config.h"
#include "wave.h"
#include <raylib.h>
#include <raymath.h>

GameState game_state_init_system() {
    GameState st;
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    SetTargetFPS(120);
    st.allocator = arena_new(1024 * 4);
    st.stage = default_stage();
    st.player = ecs_player_new();
    st.current_wave = default_wave();
    return st;
}

void game_state_update_system(GameState *state) {
    for (int i = 0; i < state->current_wave.count; i++) {
        ecs_enemy_update(&state->current_wave.enemies[i], &state->stage, &state->player.transform);
    }
    ecs_player_update(&state->player, &state->stage, &state->current_wave);
}

void game_state_draw_debug_stats(const GameState *state) {
    DrawFPS(10, 10);
    DrawText(TextFormat("Heap usage: %u/%u (%.2f%) Bytes", state->allocator.used, state->allocator.cap,
                        ((float)state->allocator.used * 100.0) / state->allocator.cap),
             10, 30, 20, GetColor(0x009900ff));
}

void game_state_draw_playfield_system(const GameState *state) {
    if (!state->player.state.dead) {
        DrawRectangleRec(state->player.transform.rect, WHITE);
    } else {
        DrawText("YOU DIED", 300, 350, 32, WHITE);
        DrawRectangleRec(state->player.transform.rect, BLACK);
    }
    for (int i = 0; i < state->current_wave.count; i++) {
        DrawRectangleRec(state->current_wave.enemies[i].transform.rect, BLUE);
    }
    draw_stage(&state->stage);
}

void game_state_frame_system(const GameState *state) {
    BeginDrawing();
    ClearBackground(GetColor(0x181818ff));
    game_state_draw_debug_stats(state);
    game_state_draw_playfield_system(state);
    EndDrawing();
}

void game_state_system(GameState *state) {
    game_state_update_system(state);
    game_state_frame_system(state);
}

void game_state_destroy(GameState *state) {
    arena_free(&state->allocator);
    CloseWindow();
}

Stage default_stage() {
    Stage st;
    st.count = 3;
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

EnemyWave default_wave() {
    return (EnemyWave){
        .enemies =
            {
                (ECSEnemy){
                    .transform = {.rect = {.x = 150, .y = 300, .width = 64, .height = 32}},
                    .physics = {.velocity = Vector2Zero(), .grounded = false},
                    .enemy_conf = {.speed = 5},
                },
                (ECSEnemy){
                    .transform = {.rect = {.x = 250, .y = 300, .width = 64, .height = 64}},
                    .physics = {.velocity = Vector2Zero(), .grounded = false},
                    .enemy_conf = {.speed = 10},
                },
                (ECSEnemy){
                    .transform = {.rect = {.x = 350, .y = 300, .width = 32, .height = 64}},
                    .physics = {.velocity = Vector2Zero(), .grounded = false},
                    .enemy_conf = {.speed = 15},
                },
                (ECSEnemy){
                    .transform = {.rect = {.x = 450, .y = 300, .width = 64, .height = 32}},
                    .physics = {.velocity = Vector2Zero(), .grounded = false},
                    .enemy_conf = {.speed = 8},
                },
                (ECSEnemy){
                    .transform = {.rect = {.x = 550, .y = 300, .width = 64, .height = 32}},
                    .physics = {.velocity = Vector2Zero(), .grounded = false},
                    .enemy_conf = {.speed = 2},
                },
            },
        .count = 5,
    };
}
