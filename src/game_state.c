#include "game_state.h"
#include "arena.h"
#include "bullet.h"
#include "enemy.h"
#include "player.h"
#include "static_config.h"
#include "timing_utilities.h"
#include "wave.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CLAY_IMPLEMENTATION
#include <clay/clay.h>
#include <clay/clay_raylib_renderer.c>

#define TRANSITION_TIME 0.25

#ifndef RELEASE
static double update_took = 0;
#endif

void clay_error_callback(Clay_ErrorData errorData) {
    printf("%s\n", errorData.errorText.chars);
    abort();
}

GameState game_state_init() {
    GameState st;
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    InitAudioDevice();
    SetTargetFPS(120);

    uint64_t clay_req_memory = Clay_MinMemorySize();
    st.clay_memory = Clay_CreateArenaWithCapacityAndMemory(clay_req_memory, malloc(clay_req_memory));

    Clay_Initialize(st.clay_memory, (Clay_Dimensions){.width = WINDOW_W, .height = WINDOW_H},
                    (Clay_ErrorHandler){.errorHandlerFunction = clay_error_callback});
    Clay_SetMeasureTextFunction(Raylib_MeasureText, st.font);

    st.allocator = arena_new(1024 * 4);
    st.stage = default_stage();
    st.player = ecs_player_new();
    st.phase = GP_TRANSITION;
    st.after_transition = GP_STARTMENU;
    st.font[0] = LoadFontEx("assets/fonts/iosevka medium.ttf", 48, NULL, 255);
    SetTextureFilter(st.font[0].texture, TEXTURE_FILTER_BILINEAR);
    st.player_jump_sound = LoadSound("assets/sfx/player_jump.wav");
    st.player_shoot_sound = LoadSound("assets/sfx/shoot.wav");
    st.enemy_hit_sound = LoadSound("assets/sfx/enemy_hit.wav");
    st.phase_change_sound = LoadSound("assets/sfx/menu_switch.wav");
    st.bullets = (Bullets){0};
    st.began_transition = GetTime();
    st.screen_type = IST_PLAYER_CLASS_SELECT;
    st.selected_class = PS_MOVE;
    st.wave_strength = 2;
    st.wave_number = 1;
    st.current_wave = generate_wave(2);
    st.bullets = (Bullets){0};
    st.camera = (Camera2D){
        .zoom = 1,
        .offset = (Vector2){WINDOW_W / 2.0, WINDOW_H / 2.0},
        .rotation = 0,
        .target = (Vector2){WINDOW_W / 2.0, WINDOW_H / 2.0},
    };
    st.volume_label_opacity = 0.0;
    st.target = LoadRenderTexture(WINDOW_W, WINDOW_H);
    st.pixelizer = LoadShader(NULL, "assets/shaders/pixelizer.fs");
    st.vfx_enabled = true;

    return st;
}

double screen_centered_position(double w) {
    return (WINDOW_W / 2.0) - (w / 2.0);
}
void game_state_phase_change(GameState *state, GamePhase next) {
    state->phase = GP_TRANSITION;
    state->after_transition = next;
    state->began_transition = GetTime();
    PlaySound(state->phase_change_sound);
}

void game_state_start_new_wave(GameState *state, PlayerClass new_class) {
    state->camera.zoom = 1.5;
    state->player.state.current_class = new_class;
    switch (new_class) {
    case PS_TANK:
        state->player.state.health = 10;
        break;
    case PS_MOVE:
    case PS_DAMAGE:
        state->player.state.health = 7;
    case PS_COUNT:
        break;
    }
    game_state_phase_change(state, GP_MAIN);
    state->wave_strength *= 1.1;
    state->wave_number++;
    state->current_wave = generate_wave(state->wave_strength);
    state->bullets = (Bullets){0};
    state->enemy_bullets = (Bullets){0};
}

void game_state_update(GameState *state) {
    Vector2 mousePosition = GetMousePosition();
    Vector2 scrollDelta = GetMouseWheelMoveV();
    Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y}, IsMouseButtonDown(0));
    Clay_UpdateScrollContainers(true, (Clay_Vector2){scrollDelta.x, scrollDelta.y}, GetFrameTime());
    float dt = GetFrameTime();
    state->camera.zoom *= 0.98;
    state->camera.zoom = Clamp(state->camera.zoom, 1, 999);
    state->volume_label_opacity = Clamp(state->volume_label_opacity - 0.02, 0, 1);

    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        state->volume_label_opacity = 1;
        SetMasterVolume(Clamp(GetMasterVolume() - 0.05, 0, 1));
    }

    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        state->volume_label_opacity = 1;
        SetMasterVolume(Clamp(GetMasterVolume() + 0.05, 0, 1));
    }

    if (IsKeyPressed(KEY_SLASH)) {
        state->vfx_enabled = !state->vfx_enabled;
    }

    if (IsKeyPressed(KEY_PRINT_SCREEN)) {
        TakeScreenshot(TextFormat("pswitch_ss_%.2f.png", GetTime()));
    }

    switch (state->phase) {
    case GP_MAIN:
        if (IsKeyPressed(KEY_P)) {
            game_state_phase_change(state, GP_PAUSED);
            return;
        }
        for (size_t i = 0; i < state->current_wave.count; i++) {
            ecs_enemy_update(&state->current_wave.enemies[i], &state->stage, &state->player.transform, &state->bullets,
                             &state->enemy_hit_sound, PLAYER_STATES[state->player.state.current_class].damage,
                             &state->enemy_bullets);
        }
        ecs_player_update(&state->player, &state->stage, &state->current_wave, &state->bullets,
                          &state->player_jump_sound, &state->player_shoot_sound, &state->enemy_bullets);
        bullets_update(&state->bullets, dt);
        bullets_update(&state->enemy_bullets, dt);
        if (state->player.state.dead) {
            game_state_phase_change(state, GP_DEAD);
        }

        if (wave_is_done(&state->current_wave)) {
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
            state->current_wave = generate_wave(state->wave_strength);
        }
        break;

    case GP_PAUSED:
        if (IsKeyPressed(KEY_P)) {
            game_state_phase_change(state, GP_MAIN);
        }
        break;
    case GP_TRANSITION:
        if (time_delta(state->began_transition) > TRANSITION_TIME) {
            state->phase = state->after_transition;
            return;
        }
        break;
        /*const double t = (GetTime() - state->began_transition) * 2;*/
    case GP_AFTER_WAVE:

        if (IsKeyDown(KEY_ENTER)) {
            game_state_start_new_wave(state, state->selected_class);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){30, 30, 128, 64})) {
                state->screen_type = IST_PLAYER_CLASS_SELECT;
            }
            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){30, 124, 128, 64})) {
                state->screen_type = IST_PLAYER_UPGRADE;
            }
        }
    }
}

void game_state_draw_playfield(const GameState *state) {
    draw_stage(&state->stage);
    wave_draw(&state->current_wave);
    bullets_draw(&state->bullets);
    bullets_draw(&state->enemy_bullets);
    player_draw(&state->player);
}

void ui_label(const char *text, uint16_t size, Color c) {
    Clay_String str = {.chars = text, .length = strlen(text)};
    CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                                        .fontId = 0,
                                        .textColor = {c.r, c.g, c.b, c.a},
                                        .fontSize = size,
                                    }, ));
}

void handle_screen_select_button_class(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->screen_type = IST_PLAYER_CLASS_SELECT;
    }
}
void handle_screen_select_button_upgrade(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->screen_type = IST_PLAYER_UPGRADE;
    }
}
void handle_player_class_select_button_tank(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->selected_class = PS_TANK;
    }
}
void handle_player_class_select_button_move(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->selected_class = PS_MOVE;
    }
}
void handle_player_class_select_button_killer(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->selected_class = PS_DAMAGE;
    }
}
void handle_speed_upgrade_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->player.state.movement_speed += 5.0;
    }
}
void handle_reload_speed_upgrade_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->player.state.reload_time += 0.02;
    }
}

Clay_Color button_color(bool activecond) {
    return activecond ? (Clay_Color){120, 120, 120, 255} : (Clay_Color){90, 90, 90, 255};
}

Clay_RenderCommandArray game_state_draw_ui(const GameState *state) {
    Clay_BeginLayout();
    CLAY({
             .id = CLAY_ID("OuterContainer"),
             .layout =
                 {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing =
                         {
                             .height = CLAY_SIZING_GROW(0),
                             .width = CLAY_SIZING_GROW(0),
                         },
                 },
         }, ) {
        switch (state->phase) {
        case GP_MAIN: {
            CLAY({
                .id = CLAY_ID("PlayerInfoContainer"),
                .layout =
                    {
                        .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = {16, 16, 16, 16},
                    },
            }) {
                ui_label(TextFormat("Health: %d", state->player.state.health), 48, WHITE);
                ui_label(TextFormat("Wave #%d", state->wave_number), 48, WHITE);
            }
            break;
        }
        case GP_STARTMENU: {
            CLAY({
                .id = CLAY_ID("StartMenuContainer"),
                .layout =
                    {
                        .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = {16, 16, 16, 16},
                    },
            }) {
                ui_label("Press `space` to start the game", 48, WHITE);
                ui_label(" ", 48, WHITE);
                ui_label("Controls:", 36, WHITE);
                ui_label("A: Move left", 36, WHITE);
                ui_label("D: Move right", 36, WHITE);
                ui_label("Space: Jump", 24, WHITE);
                ui_label("Left arrow: Shoot to the left", 24, WHITE);
                ui_label("Right arrow: Shoot to the right", 24, WHITE);
                ui_label("P: Pause the game", 24, WHITE);
                ui_label("Left bracket: Decrease master volume by 5%", 24, WHITE);
                ui_label("Right bracket: Increase master volume by 5%", 24, WHITE);
                ui_label("Slash: Toggle shaders OwO", 24, WHITE);
                ui_label("Print screen: Take screenshot", 24, WHITE);
            }
            break;
        }
        case GP_DEAD: {
            CLAY({.layout = {
                      .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                      .layoutDirection = CLAY_TOP_TO_BOTTOM,
                  }}) {
                CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                }
                CLAY({
                    .id = CLAY_ID("DeadStatusContainer"),
                    .layout =
                        {
                            .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .padding = {16, 16, 16, 16},
                        },
                }) {
                    CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                    }
                    ui_label("You died!", 48, WHITE);
                    CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                    }
                }
                CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                }
            }
            break;
        }
        case GP_AFTER_WAVE: {
            CLAY({
                .layout =
                    {
                        .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = {16, 16, 16, 16},
                    },
            }) {
                ui_label("Press enter to start the next wave", 32, WHITE);
                CLAY({
                    .id = CLAY_ID("ScreenSelectButtonContainer"),
                    .layout =
                        {
                            .sizing = {.width = CLAY_SIZING_FIXED(128), .height = CLAY_SIZING_GROW(0)},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .childGap = 32,
                        },
                }) {
                    CLAY({
                             .id = CLAY_ID("ClassSelectScreenButton"),
                             .layout =
                                 {
                                     .sizing = {.width = CLAY_SIZING_FIXED(128), .height = CLAY_SIZING_GROW(0)},
                                 },
                             .backgroundColor = button_color(state->screen_type == IST_PLAYER_CLASS_SELECT),
                         }, ) {
                        Clay_OnHover(handle_screen_select_button_class, (intptr_t)state);
                        ui_label("Class select", 24, WHITE);
                    }
                    CLAY({
                        .id = CLAY_ID("UpgradeSelectScreenButton"),
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_FIXED(128), .height = CLAY_SIZING_GROW(0)},
                            },
                        .backgroundColor = button_color(state->screen_type == IST_PLAYER_UPGRADE),
                    }) {
                        Clay_OnHover(handle_screen_select_button_upgrade, (intptr_t)state);
                        ui_label("Upgrade", 24, WHITE);
                    }
                }
            }
            switch (state->screen_type) {
            case IST_PLAYER_CLASS_SELECT: {
                CLAY({
                    .id = CLAY_ID("PlayerClassSelectContainer"),
                    .layout =
                        {
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .sizing =
                                {
                                    .width = CLAY_SIZING_GROW(0),
                                    .height = CLAY_SIZING_GROW(0),
                                },
                            .childGap = 16,
                            .padding = {16, 16, 16, 16},
                        },
                }) {
                    CLAY({
                        .id = CLAY_ID("TankClassButton"),
                        .backgroundColor = button_color(state->selected_class == PS_TANK),
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            },
                    }) {
                        Clay_OnHover(handle_player_class_select_button_tank, (intptr_t)state);
                        ui_label("Tank", 32, WHITE);
                    }
                    CLAY({
                        .id = CLAY_ID("MoveClassButton"),
                        .backgroundColor = button_color(state->selected_class == PS_MOVE),
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            },
                    }) {
                        Clay_OnHover(handle_player_class_select_button_move, (intptr_t)state);
                        ui_label("Mover", 32, WHITE);
                    }
                    CLAY({
                        .id = CLAY_ID("KillerClassButton"),
                        .backgroundColor = button_color(state->selected_class == PS_DAMAGE),
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            },
                    }) {
                        Clay_OnHover(handle_player_class_select_button_killer, (intptr_t)state);
                        ui_label("Killer", 32, WHITE);
                    }
                }
                break;
            }
            case IST_PLAYER_UPGRADE: {
                CLAY({
                    .id = CLAY_ID("PlayerUpgradeContainer"),
                    .layout =
                        {
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .sizing =
                                {
                                    .width = CLAY_SIZING_GROW(0),
                                    .height = CLAY_SIZING_GROW(0),
                                },
                            .childGap = 16,
                            .padding = {16, 16, 16, 16},
                        },
                }) {
                    CLAY({
                        .id = CLAY_ID("ReloadUpgradeButton"),
                        .backgroundColor = {128, 128, 128, 200},
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            },
                    }) {
                        Clay_OnHover(handle_reload_speed_upgrade_button, (intptr_t)state);
                        ui_label("Reload", 24, WHITE);
                    }
                    CLAY({
                        .id = CLAY_ID("SpeedUpgradeButton"),
                        .backgroundColor = {128, 128, 128, 200},
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            },
                    }) {
                        Clay_OnHover(handle_speed_upgrade_button, (intptr_t)state);
                        ui_label("Speed", 24, WHITE);
                    }
                }
                break;
            }
            }
            break;
        }
        default: {
        }
        }

        DrawTextEx(state->font[0], TextFormat("Volume: %.2f", GetMasterVolume() * 100), (Vector2){500, 40}, 48, 0,
                   GetColor(0xffffff00 + (state->volume_label_opacity * 255)));
    }

    return Clay_EndLayout();
}

void game_state_frame(GameState *state) {
    BeginTextureMode(state->target);
    BeginMode2D(state->camera);
    ClearBackground(GetColor(0x181818ff));
    switch (state->phase) {
    case GP_MAIN:
        game_state_draw_playfield(state);
        break;
    case GP_STARTMENU:
    case GP_DEAD:
        break;
    case GP_PAUSED:
        game_state_draw_playfield(state);
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
        break;
    }
    case GP_AFTER_WAVE:
        game_state_draw_playfield(state);
    }

    EndMode2D();
    EndTextureMode();

    BeginDrawing();
    ClearBackground(GetColor(0x181818ff));
    if (state->vfx_enabled) {
        BeginShaderMode(state->pixelizer);
    }
    DrawTextureRec(state->target.texture,
                   (Rectangle){
                       0,
                       (float)state->target.texture.height,
                       (float)state->target.texture.width,
                       -(float)state->target.texture.height,
                   },
                   (Vector2){0, 0}, WHITE);
    if (state->vfx_enabled) {
        EndShaderMode();
    }
    Clay_Raylib_Render(game_state_draw_ui(state), &state->font[0]);
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
    UnloadFont(state->font[0]);
    UnloadRenderTexture(state->target);
    UnloadShader(state->pixelizer);

    CloseAudioDevice();
    CloseWindow();
}

void draw_centered_text(const char *message, const Font *font, size_t size, Color color, float y) {
    const Vector2 text_size = MeasureTextEx(*font, message, size, 0);
    DrawTextEx(*font, message, (Vector2){screen_centered_position(text_size.x), y}, size, 0, color);
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

#define FAST_WEAK_ENEMY(x, y) ecs_basic_enemy((Vector2){(x), (y)}, (Vector2){32, 64}, 40, 3)
#define SLOW_STRONG_ENEMY(x, y) ecs_basic_enemy((Vector2){(x), (y)}, (Vector2){64, 64}, 20, 10)
#define RANGER(x, y) ecs_ranger_enemy((Vector2){(x), (y)}, (Vector2){32, 96}, 20, 10, 3)

EnemyWave generate_wave(double strength) {
    EnemyWave wave = {.count = 0};
    size_t current_index = 0;

    while (strength > 0) {
        size_t enemy_type = GetRandomValue(0, 2);
        switch (enemy_type) {
        case 0: {
            wave.enemies[current_index] = FAST_WEAK_ENEMY(GetRandomValue(0, 700), GetRandomValue(100, 300));
            strength -= 0.5;
            break;
        }
        case 1: {
            wave.enemies[current_index] = SLOW_STRONG_ENEMY(GetRandomValue(0, 700), GetRandomValue(100, 300));
            strength -= 1;
            break;
        }
        case 2: {
            wave.enemies[current_index] = RANGER(GetRandomValue(0, 700), GetRandomValue(100, 300));
            strength -= 1;
            break;
        }
        }
        current_index++;
    }
    wave.count = current_index;
    return wave;
}
