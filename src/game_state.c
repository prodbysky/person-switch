#include "game_state.h"
#include "static_config.h"

GameState game_state_init_system() {
    GameState st;
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    SetTargetFPS(120);
    st.allocator = arena_new(1024 * 4);
    st.stage = default_stage(&st.allocator);
    st.player = ecs_player_new();
    st.test_enemy = ecs_enemy_new((Vector2){100, 100}, (Vector2){64, 64}, 20);
    return st;
}

void game_state_update_system(GameState *state) {
    ecs_player_update(&state->player, &state->stage);
    ecs_enemy_update(&state->test_enemy, &state->stage, &state->player.transform);
}

void game_state_draw_debug_stats(const GameState *state) {
    DrawFPS(10, 10);
    DrawText(TextFormat("Heap usage: %u/%u (%.2f%) Bytes", state->allocator.used, state->allocator.cap,
                        ((float)state->allocator.used * 100.0) / state->allocator.cap),
             10, 30, 20, GetColor(0x009900ff));
}

void game_state_draw_playfield_system(const GameState *state) {
    DrawRectangleRec(state->player.transform.rect, WHITE);
    DrawRectangleRec(state->test_enemy.transform.rect, BLUE);
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
