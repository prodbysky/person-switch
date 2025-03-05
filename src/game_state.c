#include "game_state.h"
#include "ecs.h"
#include "enemy.h"
#include "player.h"
#include "static_config.h"
#include "wave.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

GameState game_state_init_system() {
    GameState st;
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    SetTargetFPS(120);
    st.allocator = arena_new(1024 * 4);
    st.stage = default_stage();
    st.player = ecs_player_new();
    st.current_wave = default_wave();
    st.phase = GP_STARTMENU;
    st.font = LoadFontEx("assets/fonts/iosevka medium.ttf", 32, NULL, 255);
    st.bullets = (Bullets){0};
    return st;
}

void game_state_update_system(GameState *state) {
    float dt = GetFrameTime();
    switch (state->phase) {
    case GP_MAIN:
        if (IsKeyPressed(KEY_P)) {
            state->phase = GP_PAUSED;
            return;
        }
        for (int i = 0; i < state->current_wave.count; i++) {
            ecs_enemy_update(&state->current_wave.enemies[i], &state->stage, &state->player.transform, &state->bullets);
        }
        ecs_player_update(&state->player, &state->stage, &state->current_wave, &state->bullets);
        bullets_update_system(&state->bullets, dt);
        if (state->player.state.dead) {
            state->phase = GP_DEAD;
        }
        break;

    case GP_STARTMENU:
        if (IsKeyPressed(KEY_SPACE)) {
            state->phase = GP_MAIN;
        }
        break;

    case GP_DEAD:
        if (IsKeyPressed(KEY_SPACE)) {
            state->phase = GP_MAIN;
            state->player = ecs_player_new();
            state->stage = default_stage();
            state->current_wave = default_wave();
        }
        break;

    case GP_PAUSED:
        if (IsKeyPressed(KEY_P)) {
            state->phase = GP_MAIN;
            return;
        }
        break;
    }
}

void game_state_draw_debug_stats(const GameState *state) {
    DrawTextEx(state->font, TextFormat("FPS: %d", GetFPS()), (Vector2){10, 10}, 32, 0, WHITE);
    DrawTextEx(state->font,
               TextFormat("Heap usage: %u/%u (%.2f%) Bytes", state->allocator.used, state->allocator.cap,
                          ((float)state->allocator.used * 100.0) / state->allocator.cap),
               (Vector2){10, 40}, 32, 0, WHITE);
}

void game_state_draw_playfield_system(const GameState *state) {
    if (!state->player.state.dead) {
        DrawRectangleRec(state->player.transform.rect, WHITE);
    }
    for (int i = 0; i < state->current_wave.count; i++) {
        if (!state->current_wave.enemies[i].dead) {
            DrawRectangleRec(state->current_wave.enemies[i].transform.rect, BLUE);
        }
    }
    draw_stage(&state->stage);
    bullets_draw_system(&state->bullets);
}

void game_state_frame_system(const GameState *state) {
    BeginDrawing();
    ClearBackground(GetColor(0x181818ff));
    switch (state->phase) {
    case GP_MAIN:
        game_state_draw_debug_stats(state);
        game_state_draw_playfield_system(state);
        break;
    case GP_STARTMENU:
        ClearBackground(GetColor(0x8a8a8aff));
        DrawTextEx(state->font, "Press `space` to start\nthe game!", (Vector2){10, 10}, 32, 0, WHITE);
        DrawTextEx(state->font, "Controls:", (Vector2){10, 90}, 32, 0, WHITE);
        DrawTextEx(state->font, "A: Move left", (Vector2){30, 120}, 32, 0, WHITE);
        DrawTextEx(state->font, "D: Move right", (Vector2){30, 150}, 32, 0, WHITE);
        DrawTextEx(state->font, "Space: Jump", (Vector2){30, 180}, 32, 0, WHITE);
        DrawTextEx(state->font, "Left arrow: Shoot to the left", (Vector2){30, 210}, 32, 0, WHITE);
        DrawTextEx(state->font, "Right arrow: Shoot to the right", (Vector2){30, 240}, 32, 0, WHITE);
        DrawTextEx(state->font, "P: Pause the game", (Vector2){30, 270}, 32, 0, WHITE);
        break;

    case GP_DEAD:
        DrawTextEx(state->font, "You died!", (Vector2){300, 350}, 32, 0, WHITE);
        break;
    case GP_PAUSED:
        game_state_draw_playfield_system(state);
        DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = WINDOW_W, .height = WINDOW_H}, Vector2Zero(), 0,
                         GetColor(0x00000055));
        game_state_draw_debug_stats(state);
        DrawTextEx(state->font, "The game is paused", (Vector2){200, 350}, 32, 0, WHITE);
        break;
    }

    EndDrawing();
}

void game_state_system(GameState *state) {
    game_state_update_system(state);
    game_state_frame_system(state);
}

void game_state_destroy(GameState *state) {
    arena_free(&state->allocator);
    UnloadFont(state->font);
    CloseWindow();
}

Stage default_stage() {
    Stage st;
    st.count = 1;
    st.platforms[0] = (Platform){
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
                (ECSEnemy){.transform = TRANSFORM(150, 300, 64, 32),
                           .physics = DEFAULT_PHYSICS(),
                           .enemy_conf = {.speed = 5},
                           .health = 5,
                           .last_hit = 0.0,
                           .dead = false},
            },
        .count = 1,
    };
}
