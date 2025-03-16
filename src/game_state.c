#include "game_state.h"
#include "bullet.h"
#include "enemy.h"
#include "pickup.h"
#include "player.h"
#include "stage.h"
#include "static_config.h"
#include "timing_utilities.h"
#include "wave.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <clay/clay.h>
#include <clay/clay_raylib_renderer.c>

#define TRANSITION_TIME 0.25

#ifndef RELEASE
static double update_took = 0;
#endif

void clay_error_callback(Clay_ErrorData errorData) {
    TraceLog(LOG_ERROR, "%s", errorData.errorText.chars);
}

static Stage (*const stages[])(void) = {stage_1, stage_2, stage_3};

GameState game_state_init() {
    GameState st;
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    InitAudioDevice();
    SetWindowState(FLAG_VSYNC_HINT);
    SetExitKey(0);

    uint64_t clay_req_memory = Clay_MinMemorySize();
    st.clay_memory = Clay_CreateArenaWithCapacityAndMemory(clay_req_memory, malloc(clay_req_memory));

    Clay_Initialize(st.clay_memory, (Clay_Dimensions){.width = WINDOW_W, .height = WINDOW_H},
                    (Clay_ErrorHandler){.errorHandlerFunction = clay_error_callback});
    Clay_SetMeasureTextFunction(Raylib_MeasureText, st.font);

#ifndef RELEASE
    Clay_SetDebugModeEnabled(true);
#endif

    st.stage = stages[0]();
    st.selected_stage = 0;
    st.player = ecs_player_new();
    st.reload_cost = 3;
    st.speed_cost = 1;
    st.phase = GP_TRANSITION;
    st.after_transition = GP_STARTMENU;
    st.font[0] = LoadFontEx("assets/fonts/iosevka medium.ttf", 48, NULL, 255);
    SetTextureFilter(st.font[0].texture, TEXTURE_FILTER_BILINEAR);
    st.ui_button_click_sound = LoadSound("assets/sfx/button_click.wav");
    st.player_jump_sound = LoadSound("assets/sfx/player_jump.wav");
    st.player_shoot_sound = LoadSound("assets/sfx/shoot.wav");
    st.enemy_hit_sound = LoadSound("assets/sfx/enemy_hit.wav");
    st.enemy_die_sound = LoadSound("assets/sfx/enemy_die.wav");
    st.phase_change_sound = LoadSound("assets/sfx/menu_switch.wav");
    st.bullets = (Bullets){.bullets = {0}, .current = 0};
    st.began_transition = GetTime();
    st.screen_type = IST_PLAYER_CLASS_SELECT;
    st.selected_class = PS_MOVE;
    st.wave_strength = 2;
    st.wave_number = 1;
    st.current_wave = generate_wave(2);
    st.enemy_bullets = (Bullets){.bullets = {0}, .current = 0};
    st.camera = (Camera2D){
        .zoom = 1,
        .offset = (Vector2){WINDOW_W / 2.0, WINDOW_H / 2.0},
        .rotation = 0,
        .target = (Vector2){WINDOW_W / 2.0, WINDOW_H / 2.0},
    };
    st.volume_label_opacity = 0.0;
    st.vfx_indicator_opacity = 0.0;
    st.raw_frame_buffer = LoadRenderTexture(WINDOW_W, WINDOW_H);
    st.final_frame_buffer = LoadRenderTexture(WINDOW_W, WINDOW_H);
    st.pixelizer = LoadShader(NULL, "assets/shaders/pixelizer.fs");
    st.vfx_enabled = true;
    st.main_menu_type = MMT_START;
    st.pickups = (Pickups){0};
    st.error = "ERROR";
    st.error_opacity = 0;

    return st;
}

double screen_centered_position(double w) {
    return (WINDOW_W / 2.0) - (w / 2.0);
}
void game_state_phase_change(GameState *state, GamePhase next) {
    if (next == GP_MAIN) {
        state->stage = stages[state->selected_stage]();
    }
    state->phase = GP_TRANSITION;
    state->after_transition = next;
    state->began_transition = GetTime();
    PlaySound(state->phase_change_sound);
}

void game_state_start_new_wave(GameState *state, PlayerClass new_class) {
    state->camera.zoom = 1.5;
    state->player.state.current_class = new_class;
    game_state_phase_change(state, GP_MAIN);
    state->bullets = (Bullets){.bullets = {0}, .current = 0};
    state->enemy_bullets = (Bullets){.bullets = {0}, .current = 0};
}

void game_state_update(GameState *state) {
    Vector2 mousePosition = GetMousePosition();
    Vector2 scrollDelta = GetMouseWheelMoveV();
    Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y}, IsMouseButtonDown(0));
    Clay_UpdateScrollContainers(true, (Clay_Vector2){scrollDelta.x, scrollDelta.y}, GetFrameTime());

    float dt = GetFrameTime();
    state->camera.zoom *= 0.98;
    state->camera.zoom = Clamp(state->camera.zoom, 1, 999);
    state->volume_label_opacity = Clamp(state->volume_label_opacity / 1.01, 0, 1);
    state->vfx_indicator_opacity = Clamp(state->vfx_indicator_opacity / 1.01, 0, 1);
    state->error_opacity = Clamp(state->error_opacity / 1.05, 0, 1);

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
        state->vfx_indicator_opacity = 1.0;
    }

    if (IsKeyPressed(KEY_PRINT_SCREEN)) {
        TakeScreenshot(TextFormat("pswitch_ss_%.2f.png", GetTime()));
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(state->ui_button_click_sound);
    }

    switch (state->phase) {
    case GP_MAIN:
        if (IsKeyPressed(KEY_ESCAPE)) {
            game_state_phase_change(state, GP_PAUSED);
            return;
        }
        for (size_t i = 0; i < state->current_wave.count; i++) {
            ecs_enemy_update(&state->current_wave.enemies[i], &state->stage, &state->player.transform, &state->bullets,
                             &state->enemy_hit_sound, &state->enemy_die_sound,
                             PLAYER_STATES[state->player.state.current_class].damage, &state->enemy_bullets,
                             &state->pickups);
        }
        ecs_player_update(&state->player, &state->stage, &state->current_wave, &state->bullets,
                          &state->player_jump_sound, &state->player_shoot_sound, &state->enemy_bullets,
                          &state->pickups);
        bullets_update(&state->bullets, dt);
        bullets_update(&state->enemy_bullets, dt);
        pickups_update(&state->pickups, &state->stage, dt);
        if (state->player.state.dead) {
            game_state_phase_change(state, GP_DEAD);
        }

        if (wave_is_done(&state->current_wave)) {
            if (IsKeyPressed(KEY_ENTER)) {
                game_state_phase_change(state, GP_AFTER_WAVE);

                state->wave_strength *= 1.1;
                state->wave_number++;
                state->current_wave = generate_wave(state->wave_strength);
            }
        }
        break;
    case GP_STARTMENU:
        break;
    case GP_DEAD:
        if (IsKeyPressed(KEY_SPACE)) {
            game_state_phase_change(state, GP_MAIN);
            state->began_transition = GetTime();
            state->player = ecs_player_new();
            state->stage = stage_2();
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
    case GP_AFTER_WAVE:
        if (IsKeyDown(KEY_ENTER)) {
            game_state_start_new_wave(state, state->selected_class);
        }
    }
}

void game_state_draw_playfield(const GameState *state) {
    draw_stage(&state->stage);
    wave_draw(&state->current_wave);
    bullets_draw(&state->bullets);
    bullets_draw(&state->enemy_bullets);
    player_draw(&state->player);
    pickups_draw(&state->pickups);
}

void ui_label(const char *text, uint16_t size, Color c, Clay_TextAlignment aligment) {
    Clay_String str = {.chars = text, .length = strlen(text)};
    CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                                        .fontId = 0,
                                        .textColor = {c.r, c.g, c.b, c.a},
                                        .fontSize = size,
                                        .textAlignment = aligment,
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
        if (state->player.state.coins >= state->speed_cost) {
            state->player.state.movement_speed += 5.0;
            state->player.state.coins -= state->speed_cost;
            state->speed_cost *= 1.1;
        }
    }
}
void handle_reload_speed_upgrade_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->player.state.coins >= state->reload_cost) {
            state->player.state.reload_time += 0.02;
            state->player.state.coins -= state->reload_cost;
            state->reload_cost *= 1.1;
        }
    }
}

void handle_continue_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        game_state_phase_change(state, GP_MAIN);
    }
}
void handle_start_game_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        game_state_phase_change(state, GP_MAIN);
        state->stage = stages[state->selected_stage]();
    }
}

void handle_stage_1_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->selected_stage = 0;
    }
}

void handle_stage_2_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->selected_stage = 1;
    }
}
void handle_stage_3_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->selected_stage = 2;
    }
}

void handle_save_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    (void)state;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        const size_t size =
            sizeof(ECSPlayer) + sizeof(Pickups) + sizeof(EnemyWave) + sizeof(double) + sizeof(size_t) + sizeof(size_t);
        uint8_t *data = calloc(size, 1);
        size_t cursor = 0;
        memcpy(data, &state->player, sizeof(ECSPlayer));
        cursor += sizeof(ECSPlayer);
        memcpy(data + cursor, &state->pickups, sizeof(Pickups));
        cursor += sizeof(Pickups);
        memcpy(data + cursor, &state->current_wave, sizeof(EnemyWave));
        cursor += sizeof(EnemyWave);
        memcpy(data + cursor, &state->wave_strength, sizeof(double));
        cursor += sizeof(double);
        memcpy(data + cursor, &state->wave_number, sizeof(size_t));
        cursor += sizeof(size_t);
        memcpy(data + cursor, &state->selected_stage, sizeof(size_t));
        cursor += sizeof(size_t);
        SaveFileData("savefile.bin", data, size);
        free(data);
    }
}

void handle_continue_game_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    (void)state;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        int got_size = 0;
        if (FileExists("savefile.bin")) {
            uint8_t *data = LoadFileData("savefile.bin", &got_size);
            uint8_t *data_unmoved = data;
            memcpy(&state->player, data, sizeof(ECSPlayer));
            data += sizeof(ECSPlayer);
            memcpy(&state->pickups, data, sizeof(Pickups));
            data += sizeof(Pickups);
            memcpy(&state->current_wave, data, sizeof(EnemyWave));
            data += sizeof(EnemyWave);
            memcpy(&state->wave_strength, data, sizeof(double));
            data += sizeof(double);
            memcpy(&state->wave_number, data, sizeof(size_t));
            data += sizeof(size_t);
            memcpy(&state->selected_stage, data, sizeof(size_t));
            data += sizeof(size_t);
            game_state_phase_change(state, GP_MAIN);
            MemFree(data_unmoved);
        } else {
            flash_error(state, "Save file not found");
        }
    }
}

void handle_begin_game_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->main_menu_type = MMT_CONFIGGAME;
    }
}
void handle_main_menu_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        game_state_phase_change(state, GP_STARTMENU);
    }
}
void handle_show_controls_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->main_menu_type = MMT_CONTROLS;
    }
}
void handle_show_main_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->main_menu_type = MMT_START;
    }
}

Clay_Color button_color(bool activecond) {
    return activecond ? (Clay_Color){120, 120, 120, 200} : (Clay_Color){90, 90, 90, 100};
}

#define LABELED_BUTTON(w, h, text, id_, callback, highlight_condition)                                                 \
    CLAY({                                                                                                             \
             .id = CLAY_ID(id_),                                                                                       \
             .layout =                                                                                                 \
                 {                                                                                                     \
                     .sizing = {.width = w, .height = h},                                                              \
                 },                                                                                                    \
             .backgroundColor = button_color(highlight_condition),                                                     \
             .cornerRadius = {10, 10, 10, 10},                                                                         \
         }, ) {                                                                                                        \
        Clay_OnHover(callback, (intptr_t)state);                                                                       \
        CENTERED_ELEMENT(ui_label(text, 36, WHITE, CLAY_TEXT_ALIGN_CENTER));                                           \
    }

#define CENTERED_ELEMENT(component)                                                                                    \
    CLAY({.layout = {                                                                                                  \
              .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},                                 \
              .layoutDirection = CLAY_TOP_TO_BOTTOM,                                                                   \
          }}) {                                                                                                        \
        CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {                                     \
        }                                                                                                              \
        CLAY({                                                                                                         \
            .layout =                                                                                                  \
                {                                                                                                      \
                    .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},                           \
                    .padding = {16, 16, 16, 16},                                                                       \
                },                                                                                                     \
        }) {                                                                                                           \
            CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {                                 \
            }                                                                                                          \
            component;                                                                                                 \
            CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {                                 \
            }                                                                                                          \
        }                                                                                                              \
        CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {                                     \
        }                                                                                                              \
    }

Clay_RenderCommandArray game_state_draw_ui(const GameState *state) {
    Clay_BeginLayout();
    CLAY({.id = CLAY_ID("OuterContainer"),
          .layout = {
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
              .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
          }}) {
        switch (state->phase) {
        case GP_MAIN: {
            CLAY({.id = CLAY_ID("PlayerInfoContainer"),
                  .layout = {
                      .sizing = {.height = CLAY_SIZING_PERCENT(0.4), .width = CLAY_SIZING_PERCENT(0.4)},
                      .layoutDirection = CLAY_TOP_TO_BOTTOM,
                      .padding = {16, 16, 16, 16},
                  }}) {
                ui_label(TextFormat("Health: %d", state->player.state.health), 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                ui_label(TextFormat("Coins: %.2f", state->player.state.coins), 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                ui_label(TextFormat("Wave #%d", state->wave_number), 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
            }
            if (wave_is_done(&state->current_wave)) {
                CENTERED_ELEMENT(
                    ui_label("Press enter to enter the intermission screen", 36, WHITE, CLAY_TEXT_ALIGN_CENTER));
            }
            break;
        }
        case GP_STARTMENU: {
            CLAY({.id = CLAY_ID("StartMenuContainer"),
                  .layout = {
                      .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                      .layoutDirection = CLAY_TOP_TO_BOTTOM,
                      .padding = {16, 16, 16, 16},
                      .childGap = 16,
                  }}) {
                switch (state->main_menu_type) {
                case MMT_START: {
                    CLAY({.id = CLAY_ID("TitleTextContainer"),
                          .layout = {
                              .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                          }}) {
                        ui_label("Persona", 48, WHITE, CLAY_TEXT_ALIGN_CENTER);
                    };
                    CLAY({.id = CLAY_ID("TitleScreenButtonContainer"),
                          .layout = {.sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                     .childGap = 16}}) {
                        CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                        }
                        LABELED_BUTTON(CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_PERCENT(0.3), "New", "NewButton",
                                       handle_begin_game_button, false);
                        CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                        }
                        LABELED_BUTTON(CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_PERCENT(0.3), "Controls",
                                       "ShowControlsButton", handle_show_controls_button, false);
                        CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                        }
                        LABELED_BUTTON(CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_PERCENT(0.3), "Continue", "ContinueButton",
                                       handle_continue_game_button, false);
                        CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}}) {
                        }
                    }
                    break;
                }
                case MMT_CONTROLS: {
                    CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_PERCENT(0.25), CLAY_SIZING_GROW(0), "Go back",
                                                    "GoBackButton", handle_show_main_button, false));
                    CLAY({.id = CLAY_ID("ControlsListingContainer"),
                          .layout =
                              {
                                  .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
                              },
                          .scroll = {.vertical = true}}) {
                        ui_label("A: Move left", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("D: Move right", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Space: Jump", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Left arrow: Shoot to the left", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Right arrow: Shoot to the right", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Escape: Pause the game", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Left bracket: Decrease master volume by 5%", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Right bracket: Increase master volume by 5%", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Slash: Toggle shaders OwO", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label("Print screen: Take screenshot", 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                    }
                    break;
                }
                case MMT_CONFIGGAME: {
                    CLAY({.layout =
                              {
                                  .sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
                              }

                    }) {
                        CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Stage 1",
                                                        "Stage1Button", handle_stage_1_button,
                                                        state->selected_stage == 0));
                        CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Stage 2",
                                                        "Stage2Button", handle_stage_2_button,
                                                        state->selected_stage == 1));
                        CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Stage 3",
                                                        "Stage3Button", handle_stage_3_button,
                                                        state->selected_stage == 2));
                    }
                    CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_PERCENT(0.25), CLAY_SIZING_GROW(0), "Play",
                                                    "PlayButton", handle_start_game_button, false));

                    CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_PERCENT(0.25), CLAY_SIZING_GROW(0), "Go back",
                                                    "GoBackButton", handle_show_main_button, false));
                    break;
                }
                }
            }
            break;
        }
        case GP_DEAD: {
            CENTERED_ELEMENT(ui_label("You died!", 48, WHITE, CLAY_TEXT_ALIGN_CENTER));
            break;
        }
        case GP_AFTER_WAVE: {
            CLAY({.id = CLAY_ID("IntermissionScreenContainer"),
                  .layout = {.sizing = {.height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0)},
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .padding = {16, 16, 16, 16},
                             .childGap = 16}}) {
                CLAY({.id = CLAY_ID("ScreenSelectButtonContainer"),
                      .layout = {.sizing = {.width = CLAY_SIZING_FIXED(128), .height = CLAY_SIZING_GROW(0)},
                                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 .childGap = 16}}) {
                    LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Class select", "ClassSelectScreenButton",
                                   handle_screen_select_button_class, state->screen_type == IST_PLAYER_CLASS_SELECT);
                    LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Upgrades", "UpgradeSelectScreenButton",
                                   handle_screen_select_button_upgrade, state->screen_type == IST_PLAYER_UPGRADE);
                }

                CLAY({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 .childGap = 16}}) {
                    CLAY({.id = CLAY_ID("NextWaveStartGuideContainer"),
                          .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.3)},
                                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                     .childGap = 16},
                          .backgroundColor = {100, 100, 100, 255},
                          .cornerRadius = {5, 5, 5, 5}}) {
                        CENTERED_ELEMENT(
                            ui_label("Press enter to start the next wave", 50, WHITE, CLAY_TEXT_ALIGN_CENTER));
                    }

                    CLAY({.id = CLAY_ID("NextWaveEnemyListContainer"),
                          .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                     .childGap = 8,
                                     .padding = {16, 16, 16, 16}},
                          .backgroundColor = {100, 100, 100, 255},
                          .scroll = {.horizontal = false, .vertical = true},
                          .cornerRadius = {10, 10, 10, 10}}) {
                        size_t count[ET_COUNT] = {0};
                        for (size_t i = 0; i < state->current_wave.count; i++) {
                            const ECSEnemy *e = &state->current_wave.enemies[i];
                            count[e->state.type]++;
                        }
                        ui_label(TextFormat("x%d melee enemies", count[ET_BASIC]), 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                        ui_label(TextFormat("x%d ranged enemies", count[ET_RANGER]), 36, WHITE, CLAY_TEXT_ALIGN_LEFT);
                    }
                }
                switch (state->screen_type) {
                case IST_PLAYER_CLASS_SELECT: {
                    CLAY({.id = CLAY_ID("PlayerClassSelectContainer"),
                          .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                                     .sizing =
                                         {
                                             .width = CLAY_SIZING_GROW(0),
                                             .height = CLAY_SIZING_GROW(0),
                                         },
                                     .childGap = 16}}) {
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Tank", "TankClassButton",
                                       handle_player_class_select_button_tank, state->selected_class == PS_TANK);
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Mover", "MoveClassButton",
                                       handle_player_class_select_button_move, state->selected_class == PS_MOVE);
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Killer", "KillerClassButton",
                                       handle_player_class_select_button_killer, state->selected_class == PS_DAMAGE);
                    }
                    break;
                }
                case IST_PLAYER_UPGRADE: {
                    CLAY({.id = CLAY_ID("PlayerUpgradeContainer"),
                          .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                                     .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                     .childGap = 16}}) {

                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Reload", "ReloadUpgradeButton",
                                       handle_reload_speed_upgrade_button, false);
                        ui_label(TextFormat("Cost: %.2f", state->reload_cost), 28, WHITE, CLAY_TEXT_ALIGN_CENTER);
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Speed", "SpeedUpgradeButton",
                                       handle_speed_upgrade_button, false);
                        ui_label(TextFormat("Cost: %.2f", state->speed_cost), 28, WHITE, CLAY_TEXT_ALIGN_CENTER);
                    }
                    break;
                }
                }
            }
            break;
        }
        case GP_PAUSED: {
            CLAY({.id = CLAY_ID("PauseMenuContainer"),
                  .layout = {.sizing =
                                 {
                                     .width = CLAY_SIZING_GROW(0),
                                     .height = CLAY_SIZING_GROW(0),
                                 },
                             .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .padding = {16, 16, 16, 16},
                             .childGap = 16}}) {
                LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Continue", "ContinueButton",
                               handle_continue_button, false);
                LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Save game", "SaveButton", handle_save_button,
                               false);
                LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Main menu", "MainMenuButton",
                               handle_main_menu_button, false);
            }
            break;
        }
        default: {
        }
        }

        DrawTextEx(state->font[0], TextFormat("Volume: %.2f", GetMasterVolume() * 100), (Vector2){500, 40}, 48, 0,
                   GetColor(0xffffff00 + (state->volume_label_opacity * 255)));
        if (state->vfx_enabled) {
            DrawTextEx(state->font[0], "VFX enabled", (Vector2){500, 40}, 48, 0,
                       GetColor(0xffffff00 + (state->vfx_indicator_opacity * 255)));
        } else {
            DrawTextEx(state->font[0], "VFX disabled", (Vector2){500, 40}, 48, 0,
                       GetColor(0xffffff00 + (state->vfx_indicator_opacity * 255)));
        }

        DrawTextEx(state->font[0], state->error, (Vector2){200, 200}, 60, 0,
                   GetColor(0xff000000 + (state->error_opacity * 255)));
    }

    return Clay_EndLayout();
}

void apply_shader(RenderTexture2D *in, RenderTexture2D *out, Shader *shader) {
    BeginTextureMode(*out);
    ClearBackground(BLACK);
    if (shader != NULL) {
        BeginShaderMode(*shader);
    }
    DrawTextureRec(in->texture,
                   (Rectangle){
                       0,
                       (float)in->texture.height,
                       (float)in->texture.width,
                       -(float)in->texture.height,
                   },
                   (Vector2){0, 0}, WHITE);
    if (shader != NULL) {
        EndShaderMode();
    }
    EndTextureMode();
}

void game_state_frame(GameState *state) {
    BeginTextureMode(state->raw_frame_buffer);
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
        DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = WINDOW_W, .height = WINDOW_H}, Vector2Zero(), 0,
                         GetColor(0x00000040));
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

    if (state->vfx_enabled) {
        apply_shader(&state->raw_frame_buffer, &state->final_frame_buffer, &state->pixelizer);
    } else {
        apply_shader(&state->raw_frame_buffer, &state->final_frame_buffer, NULL);
    }

    BeginDrawing();
    ClearBackground(GetColor(0x181818ff));
    DrawTextureRec(state->final_frame_buffer.texture,
                   (Rectangle){
                       0,
                       (float)state->final_frame_buffer.texture.height,
                       (float)state->final_frame_buffer.texture.width,
                       -(float)state->final_frame_buffer.texture.height,
                   },
                   (Vector2){0, 0}, WHITE);
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
    UnloadFont(state->font[0]);
    UnloadRenderTexture(state->raw_frame_buffer);
    UnloadRenderTexture(state->final_frame_buffer);
    UnloadShader(state->pixelizer);
    CloseAudioDevice();
    CloseWindow();
}

void draw_centered_text(const char *message, const Font *font, size_t size, Color color, float y) {
    const Vector2 text_size = MeasureTextEx(*font, message, size, 0);
    DrawTextEx(*font, message, (Vector2){screen_centered_position(text_size.x), y}, size, 0, color);
}

void flash_error(GameState *state, char *message) {
    state->error_opacity = 1.0;
    state->error = message;
}

Stage stage_1() {
    return (Stage){
        .count = 1,
        .platforms =
            {
                (Platform){
                    .x = 0,
                    .y = 550,
                    .width = 800,
                    .height = 32,
                },
            },
    };
}
Stage stage_2() {
    return (Stage){
        .count = 2,
        .platforms =
            {
                (Platform){
                    .x = 0,
                    .y = 550,
                    .width = 800,
                    .height = 32,
                },
                (Platform){
                    .x = 100,
                    .y = 350,
                    .width = 600,
                    .height = 32,
                },
            },
    };
}
Stage stage_3() {
    return (Stage){
        .count = 3,
        .platforms =
            {
                (Platform){
                    .x = 0,
                    .y = 550,
                    .width = 800,
                    .height = 32,
                },
                (Platform){
                    .x = 100,
                    .y = 350,
                    .width = 600,
                    .height = 32,
                },
                (Platform){
                    .x = 200,
                    .y = 150,
                    .width = 400,
                    .height = 32,
                },
            },
    };
}

#define SLOW_STRONG_ENEMY(x, y) ecs_basic_enemy((Vector2){(x), (y)}, (Vector2){64, 64}, 20, 10)
#define RANGER(x, y) ecs_ranger_enemy((Vector2){(x), (y)}, (Vector2){32, 96}, 20, 10, 3)

EnemyWave generate_wave(double strength) {
    EnemyWave wave = {.count = 0};
    size_t current_index = 0;

    while (strength > 0) {
        size_t enemy_type = GetRandomValue(0, 1);
        switch (enemy_type) {
        case 0: {
            wave.enemies[current_index] = SLOW_STRONG_ENEMY(GetRandomValue(0, 700), GetRandomValue(100, 300));
            strength -= 1;
            break;
        }
        case 1: {
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
