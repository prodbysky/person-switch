#include "game_state.h"
#include "arena.h"
#include "ecs.h"
#include "enemy.h"
#include "player.h"
#include "static_config.h"
#include "wave.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

#define TRANSITION_TIME 0.25

#ifndef RELEASE
static double update_took = 0;
#endif

GameState game_state_init() {
    GameState st;
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    InitAudioDevice();
    SetTargetFPS(120);

    st.allocator = arena_new(1024 * 4);
    st.stage = default_stage();
    st.player = ecs_player_new();
    st.current_wave = default_wave();
    st.wave_finished = false;
    st.phase = GP_TRANSITION;
    st.after_transition = GP_STARTMENU;
    st.font = LoadFontEx("assets/fonts/iosevka medium.ttf", 48, NULL, 255);
    SetTextureFilter(st.font.texture, TEXTURE_FILTER_BILINEAR);
    st.player_jump_sound = LoadSound("assets/sfx/player_jump.wav");
    st.player_shoot_sound = LoadSound("assets/sfx/shoot.wav");
    st.enemy_hit_sound = LoadSound("assets/sfx/enemy_hit.wav");
    st.phase_change_sound = LoadSound("assets/sfx/menu_switch.wav");
    st.bullets = (Bullets){0};
    st.began_transition = GetTime();

    return st;
}
void game_state_phase_change(GameState *state, GamePhase next) {
    state->phase = GP_TRANSITION;
    state->after_transition = next;
    state->began_transition = GetTime();
    PlaySound(state->phase_change_sound);
}

void game_state_update(GameState *state) {
    float dt = GetFrameTime();
    switch (state->phase) {
    case GP_MAIN:
        if (IsKeyPressed(KEY_P)) {
            game_state_phase_change(state, GP_PAUSED);
            return;
        }
        for (int i = 0; i < state->current_wave.count; i++) {
            ecs_enemy_update(&state->current_wave.enemies[i], &state->stage, &state->player.transform, &state->bullets,
                             &state->enemy_hit_sound);
        }
        ecs_player_update(&state->player, &state->stage, &state->current_wave, &state->bullets,
                          &state->player_jump_sound, &state->player_shoot_sound);
        bullets_update(&state->bullets, dt);
        if (state->player.state.dead) {
            game_state_phase_change(state, GP_DEAD);
        }
        state->wave_finished = true;
        for (int i = 0; i < state->current_wave.count; i++) {
            if (!state->current_wave.enemies[i].dead) {
                state->wave_finished = false;
            }
        }
        if (state->wave_finished) {
            state->wave_finished = false;
            game_state_phase_change(state, GP_AFTER_WAVE);
        }
        break;

    case GP_STARTMENU:
        if (IsKeyPressed(KEY_SPACE)) {
            game_state_phase_change(state, GP_MAIN);
        }
        break;

    case GP_DEAD:
        if (IsKeyPressed(KEY_SPACE)) {
            game_state_phase_change(state, GP_MAIN);
            state->began_transition = GetTime();
            state->player = ecs_player_new();
            state->stage = default_stage();
            state->current_wave = default_wave();
        }
        break;

    case GP_PAUSED:
        if (IsKeyPressed(KEY_P)) {
            game_state_phase_change(state, GP_MAIN);
        }
        break;
    case GP_TRANSITION:
        if (GetTime() - state->began_transition > TRANSITION_TIME) {
            state->phase = state->after_transition;
            return;
        }
        break;
        /*const double t = (GetTime() - state->began_transition) * 2;*/
    case GP_AFTER_WAVE:
        if (IsKeyPressed(KEY_SPACE)) {
            game_state_phase_change(state, GP_MAIN);
            state->began_transition = GetTime();
            state->stage = default_stage();
            state->current_wave = default_wave();
        }
    }
}

#ifndef RELEASE
void game_state_draw_debug_stats(const GameState *state) {
    DrawTextEx(state->font, TextFormat("Frame time: %.4f ms.", GetFrameTime()), (Vector2){10, 10}, 32, 0, WHITE);
    DrawTextEx(state->font, TextFormat("Update took: %.4f ms.", update_took * 1000), (Vector2){10, 40}, 32, 0, WHITE);
    DrawTextEx(state->font,
               TextFormat("Heap usage: %u/%u (%.2f%) Bytes", state->allocator.used, state->allocator.cap,
                          ((float)state->allocator.used * 100.0) / state->allocator.cap),
               (Vector2){10, 70}, 32, 0, WHITE);
}
#endif

void game_state_draw_playfield(const GameState *state) {
    draw_stage(&state->stage);
    for (int i = 0; i < state->current_wave.count; i++) {
        if (!state->current_wave.enemies[i].dead) {
            DrawRectangleRec(state->current_wave.enemies[i].transform.rect, state->current_wave.enemies[i].c);
        }
    }
    bullets_draw(&state->bullets);
    if (!state->player.state.dead) {
        DrawRectangleRec(state->player.transform.rect, state->player.c);
    }
}

void game_state_frame(const GameState *state) {
    BeginDrawing();
    ClearBackground(GetColor(0x181818ff));
    switch (state->phase) {
    case GP_MAIN:

#ifndef RELEASE
        game_state_draw_debug_stats(state);
#endif
        game_state_draw_playfield(state);
        break;
    case GP_STARTMENU:
        ClearBackground(GetColor(0x0a0a0aff));
        Vector2 font_24_size = MeasureTextEx(state->font, "A", 24, 0);
        const float line_spacing = font_24_size.y;
        Vector2 ui_cursor = {.x = 10, .y = 10};
        DrawTextEx(state->font, "Press `space` to start\nthe game!", ui_cursor, 32, 0, WHITE);
        ui_cursor.y = 90;
        DrawTextEx(state->font, "Controls:", ui_cursor, 24, 0, WHITE);
        ui_cursor.x += 10;
        ui_cursor.y += line_spacing;
        DrawTextEx(state->font, "A: Move left", ui_cursor, 24, 0, WHITE);
        ui_cursor.y += line_spacing;
        DrawTextEx(state->font, "D: Move right", ui_cursor, 24, 0, WHITE);
        ui_cursor.y += line_spacing;
        DrawTextEx(state->font, "Space: Jump", ui_cursor, 24, 0, WHITE);
        ui_cursor.y += line_spacing;
        DrawTextEx(state->font, "Left arrow: Shoot to the left", ui_cursor, 24, 0, WHITE);
        ui_cursor.y += line_spacing;
        DrawTextEx(state->font, "Right arrow: Shoot to the right", ui_cursor, 24, 0, WHITE);
        ui_cursor.y += line_spacing;
        DrawTextEx(state->font, "P: Pause the game", ui_cursor, 24, 0, WHITE);
        break;

    case GP_DEAD:
        DrawTextEx(state->font, "You died!", (Vector2){300, 350}, 32, 0, WHITE);
        break;
    case GP_PAUSED:
        game_state_draw_playfield(state);
        DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = WINDOW_W, .height = WINDOW_H}, Vector2Zero(), 0,
                         GetColor(0x00000055));

#ifndef RELEASE
        game_state_draw_debug_stats(state);
#endif
        break;
    case GP_TRANSITION: {
        double t = (GetTime() - state->began_transition) * 1.0 / TRANSITION_TIME;
        game_state_draw_playfield(state);
        if (t < 0.5)
            DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = WINDOW_W, .height = WINDOW_H}, Vector2Zero(), 0,
                             GetColor(0xffffff00 + (t * 255)));
        else
            DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = WINDOW_W, .height = WINDOW_H}, Vector2Zero(), 0,
                             GetColor(0xffffff40 - (t * 255)));

#ifndef RELEASE
        game_state_draw_debug_stats(state);
#endif
        break;
    }
    case GP_AFTER_WAVE:
        game_state_draw_playfield(state);
        DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = WINDOW_W, .height = WINDOW_H}, Vector2Zero(), 0,
                         GetColor(0x00000055));
        DrawTextEx(state->font, "Wave finished", (Vector2){200, 200}, 36, 0, WHITE);
#ifndef RELEASE
        game_state_draw_debug_stats(state);
#endif
    }

    EndDrawing();
}

void game_state(GameState *state) {
#ifndef RELEASE
    double pre_update = GetTime();
#endif
    game_state_update(state);
#ifndef RELEASE
    double post_update = GetTime();
    update_took = post_update - pre_update;
#endif
    game_state_frame(state);
}

void game_state_destroy(GameState *state) {
    arena_free(&state->allocator);
    UnloadFont(state->font);
    CloseAudioDevice();
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

#define FAST_WEAK_ENEMY(x, y) ecs_enemy_new((Vector2){(x), (y)}, (Vector2){32, 64}, 40, 3)
#define SLOW_STRONG_ENEMY(x, y) ecs_enemy_new((Vector2){(x), (y)}, (Vector2){64, 64}, 20, 10)

EnemyWave default_wave() {
    return (EnemyWave){
        .enemies =
            {
                FAST_WEAK_ENEMY(300, 300),
                SLOW_STRONG_ENEMY(500, 200),
            },
        .count = 2,
    };
}
