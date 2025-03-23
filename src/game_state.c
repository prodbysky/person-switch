#include "game_state.h"
#include "bullet.h"
#include "ecs.h"
#include "enemy.h"
#include "particles.h"
#include "pickup.h"
#include "player.h"
#include "stage.h"
#include "stb_ds_helper.h"
#include "timing_utilities.h"
#include "wave.h"
#include "weapon.h"
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <clay/clay.h>
#include <clay/clay_raylib_renderer.c>

#define TRANSITION_TIME 0.25

void clay_error_callback(Clay_ErrorData errorData) {
    TraceLog(LOG_ERROR, "%s", errorData.errorText.chars);
}

static Stage (*const stages[])(void) = {stage_1, stage_2, stage_3};

GameState game_state_init() {
    GameState st;
    InitWindow(100, 100, "Persona");
    InitAudioDevice();
    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowSize(GetMonitorWidth(0), GetMonitorHeight(0));
    ToggleFullscreen();
    SetExitKey(0);

    uint64_t clay_req_memory = Clay_MinMemorySize();
    st.clay_memory = Clay_CreateArenaWithCapacityAndMemory(clay_req_memory, malloc(clay_req_memory));

    Clay_Initialize(st.clay_memory, (Clay_Dimensions){GetMonitorWidth(0), GetMonitorHeight(0)},
                    (Clay_ErrorHandler){.errorHandlerFunction = clay_error_callback});
    Clay_SetMeasureTextFunction(Raylib_MeasureText, st.font);

#ifndef RELEASE
    Clay_SetDebugModeEnabled(true);
#endif
    st.particles = NULL;
    st.selected_stage = 0;
    st.player = ecs_player_new();
    st.speed_cost = 1;
    st.phase = GP_TRANSITION;
    st.after_transition = GP_STARTMENU;
    st.font[0] = LoadFontEx("assets/fonts/iosevka medium.ttf", 48, NULL, 255);
    SetTextureFilter(st.font[0].texture, TEXTURE_FILTER_BILINEAR);
    st.ui_button_click_sound = LoadSound("assets/sfx/button_click.wav");
    st.enemy_hit_sound = LoadSound("assets/sfx/enemy_hit.wav");
    st.enemy_die_sound = LoadSound("assets/sfx/enemy_die.wav");
    st.phase_change_sound = LoadSound("assets/sfx/menu_switch.wav");
    st.bullets = NULL;
    st.began_transition = GetTime();
    st.screen_type = IST_PLAYER_UPGRADE;
    st.wave_strength = 2;
    st.wave_number = 1;
    st.current_wave = NULL;
    st.enemy_bullets = NULL;
    st.camera = (Camera2D){
        .zoom = 0.75,
        .offset = (Vector2){GetMonitorWidth(0) / 2.0, GetMonitorHeight(0) / 2.0},
        .rotation = 0,
        .target = (Vector2){GetMonitorWidth(0) / 2.0, GetMonitorHeight(0) / 2.0},
    };
    st.volume_label_opacity = 0.0;
    st.vfx_indicator_opacity = 0.0;
    st.raw_frame_buffer = LoadRenderTexture(GetMonitorWidth(0), GetMonitorHeight(0));
    st.ui_frame_buffer = LoadRenderTexture(GetMonitorWidth(0), GetMonitorHeight(0));
    st.final_frame_buffer = LoadRenderTexture(GetMonitorWidth(0), GetMonitorHeight(0));
    st.pixelizer = LoadShader(NULL, "assets/shaders/pixelizer.fs");
    st.vfx_enabled = true;
    st.main_menu_type = MMT_START;
    st.pickups = (Pickups){0};
    st.error = "ERROR";
    st.error_opacity = 0;

    return st;
}
void game_state(GameState *state) {
    game_state_update(state);
    game_state_frame(state);
}

void game_state_frame(GameState *state) {
    BeginTextureMode(state->raw_frame_buffer);
    BeginMode2D(state->camera);
    ClearBackground(GetColor(0x181818ff));
    switch (state->phase) {
    case GP_PAUSED:
    case GP_TRANSITION:
    case GP_AFTER_WAVE:
    case GP_MAIN:
        game_state_draw_playfield(state);
        break;
    case GP_DEAD:
    case GP_STARTMENU:
        break;
    }

    EndMode2D();

    switch (state->phase) {
    case GP_PAUSED: {
        DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = GetMonitorWidth(0), .height = GetMonitorHeight(0)},
                         Vector2Zero(), 0, GetColor(0x00000040));
        break;
    }
    case GP_TRANSITION: {
        double t = (GetTime() - state->began_transition) * 1.0 / TRANSITION_TIME;
        if (t < 0.5)
            DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = GetMonitorWidth(0), .height = GetMonitorHeight(0)},
                             Vector2Zero(), 0, GetColor(0xffffff00 + (t * 255)));
        else
            DrawRectanglePro((Rectangle){.x = 0, .y = 0, .width = GetMonitorWidth(0), .height = GetMonitorHeight(0)},
                             Vector2Zero(), 0, GetColor(0xffffff40 - (t * 255)));
        break;
    }
    case GP_MAIN: {
        if (state->player.state.health < 3) {
            DrawRectangle(0, 0, GetMonitorWidth(0), GetMonitorHeight(0),
                          GetColor(0xff000000 + (((sinf(GetTime() * 10) + 1) / 2.0) * 40)));
        }
        break;
    }
    default: {
    }
    }

    EndTextureMode();

    if (state->vfx_enabled) {
        apply_shader(&state->raw_frame_buffer, &state->final_frame_buffer, &state->pixelizer);
    } else {
        apply_shader(&state->raw_frame_buffer, &state->final_frame_buffer, NULL);
    }

    BeginTextureMode(state->ui_frame_buffer);
    ClearBackground(GetColor(0));
    Clay_Raylib_Render(game_state_draw_ui(state), &state->font[0]);
    EndTextureMode();

    BeginDrawing();
    DrawTextureRec(state->final_frame_buffer.texture,
                   (Rectangle){
                       0,
                       (float)state->final_frame_buffer.texture.height,
                       (float)state->final_frame_buffer.texture.width,
                       -(float)state->final_frame_buffer.texture.height,
                   },
                   (Vector2){0, 0}, WHITE);
    DrawTextureRec(state->ui_frame_buffer.texture,
                   (Rectangle){
                       0,
                       (float)state->ui_frame_buffer.texture.height,
                       (float)state->ui_frame_buffer.texture.width,
                       -(float)state->ui_frame_buffer.texture.height,
                   },
                   (Vector2){0, 0}, WHITE);
    EndDrawing();
}

void game_state_update(GameState *state) {
    game_state_update_ui_internals();
    game_state_update_camera(&state->camera, &state->player.transform);

    const float dt = GetFrameTime();

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
        game_state_update_gp_main(state, dt);
        break;
    case GP_STARTMENU:
        break;
    case GP_DEAD:
        game_state_update_gp_dead(state, dt);
        break;
    case GP_PAUSED:
        game_state_update_gp_paused(state, dt);
        break;
    case GP_TRANSITION:
        game_state_update_gp_transition(state, dt);
        break;
    case GP_AFTER_WAVE:
        game_state_update_gp_after_wave(state, dt);
        break;
    }
}

void game_state_destroy(GameState *state) {
    UnloadFont(state->font[0]);
    UnloadRenderTexture(state->raw_frame_buffer);
    UnloadRenderTexture(state->ui_frame_buffer);
    UnloadRenderTexture(state->final_frame_buffer);
    UnloadShader(state->pixelizer);
    stbds_arrfreef(state->bullets);
    stbds_arrfreef(state->enemy_bullets);
    stbds_arrfreef(state->current_wave);
    stbds_arrfreef(state->pickups);
    stbds_arrfreef(state->particles);
    CloseAudioDevice();
    CloseWindow();
}

void game_state_update_gp_main(GameState *state, float dt) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        game_state_phase_change(state, GP_PAUSED);
        return;
    }
    for (ptrdiff_t i = 0; i < stbds_arrlen(state->current_wave); i++) {
        ecs_enemy_update(&state->current_wave[i], &state->stage, &state->player.transform, &state->player.physics,
                         &state->bullets, &state->enemy_hit_sound, &state->enemy_die_sound, &state->enemy_bullets,
                         &state->pickups, &state->particles);
    }
    ecs_player_update(&state->player, &state->stage, &state->current_wave, &state->bullets, &state->enemy_bullets,
                      &state->pickups, &state->camera, &state->particles);
    bullets_update(&state->bullets, dt, &state->stage, &state->particles);
    bullets_update(&state->enemy_bullets, dt, &state->stage, &state->particles);
    pickups_update(&state->pickups, &state->stage, dt);
    if (state->player.state.dead) {
        game_state_phase_change(state, GP_DEAD);
    }

    if (wave_is_done(&state->current_wave)) {
        if (IsKeyPressed(KEY_ENTER)) {
            game_state_phase_change(state, GP_AFTER_WAVE);
            STB_DS_ARRAY_RESET(state->current_wave);

            state->wave_strength *= 1.1;
            state->wave_number++;
            state->current_wave = generate_wave(state->wave_strength, &state->stage);
        }
    }
    particles_update(&state->particles, &state->stage, dt);
}

void game_state_update_gp_dead(GameState *state, float dt) {
    (void)dt;
    if (IsKeyPressed(KEY_SPACE)) {
        game_state_phase_change(state, GP_MAIN);
        state->began_transition = GetTime();
        state->player = ecs_player_new();
        state->stage = stages[state->selected_stage]();
        state->current_wave = generate_wave(state->wave_strength, &state->stage);
        state->player.transform.rect.x = state->stage.spawn.x;
        state->player.transform.rect.y = state->stage.spawn.y;
    }
}

void game_state_update_gp_paused(GameState *state, float dt) {
    (void)dt;
    if (IsKeyPressed(KEY_P)) {
        game_state_phase_change(state, GP_MAIN);
    }
}

void game_state_update_gp_transition(GameState *state, float dt) {
    (void)dt;
    if (time_delta(state->began_transition) > TRANSITION_TIME) {
        state->phase = state->after_transition;
        return;
    }
}

void game_state_update_gp_after_wave(GameState *state, float dt) {
    (void)dt;
    if (IsKeyDown(KEY_ENTER)) {
        game_state_start_new_wave(state);
    }
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

void game_state_start_new_wave(GameState *state) {
    game_state_phase_change(state, GP_MAIN);
    STB_DS_ARRAY_RESET(state->bullets);
    STB_DS_ARRAY_RESET(state->enemy_bullets);
}

void game_state_update_ui_internals() {
    Vector2 mousePosition = GetMousePosition();
    Vector2 scrollDelta = GetMouseWheelMoveV();
    Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y}, IsMouseButtonDown(0));
    Clay_UpdateScrollContainers(true, (Clay_Vector2){scrollDelta.x, scrollDelta.y}, GetFrameTime());
}

void game_state_update_camera(Camera2D *camera, const TransformComp *target) {
    const float smoothing_factor = 2.0f * GetFrameTime();
    const Vector2 mouse_position = GetMousePosition();

    Vector2 desired_camera_target = (Vector2){target->rect.x, target->rect.y};
    desired_camera_target = Vector2Add(
        desired_camera_target,
        Vector2Scale(Vector2Subtract((Vector2){GetMonitorWidth(0) / 2.0, GetMonitorHeight(0) / 2.0}, mouse_position),
                     -0.5));

    camera->target.x = Lerp(camera->target.x, desired_camera_target.x, smoothing_factor);
    camera->target.y = Lerp(camera->target.y, desired_camera_target.y, smoothing_factor);
}

void game_state_draw_playfield(const GameState *state) {
    draw_stage(&state->stage);
    wave_draw(&state->current_wave);
    bullets_draw(&state->bullets);
    bullets_draw(&state->enemy_bullets);
    player_draw(&state->player);
    pickups_draw(&state->pickups);
    particles_draw(&state->particles);

    const float arrow_length = 50.0f;
    const float arrow_thickness = 5.0f;
    Color arrow_color = GetColor(0xff000055);
    for (ptrdiff_t i = 0; i < stbds_arrlen(state->current_wave); i++) {
        if (state->current_wave[i].state.dead) {
            continue;
        }
        Vector2 direction =
            Vector2Normalize((Vector2){state->current_wave[i].transform.rect.x -
                                           (state->player.transform.rect.x + state->player.transform.rect.width / 2),
                                       state->current_wave[i].transform.rect.y -
                                           (state->player.transform.rect.y + state->player.transform.rect.height / 2)});

        Vector2 arrow_end = {
            (state->player.transform.rect.x + state->player.transform.rect.width / 2) + direction.x * arrow_length,
            (state->player.transform.rect.y + state->player.transform.rect.height / 2) + direction.y * arrow_length};

        DrawLineEx((Vector2){state->player.transform.rect.x + state->player.transform.rect.width / 2,
                             state->player.transform.rect.y + state->player.transform.rect.height / 2},
                   arrow_end, arrow_thickness, arrow_color);
    }
    arrow_color = GetColor(0x00ff0055);
    for (ptrdiff_t i = 0; i < stbds_arrlen(state->pickups); i++) {
        if (!state->pickups[i].active) {
            continue;
        }
        Vector2 direction =
            Vector2Normalize((Vector2){state->pickups[i].transform.rect.x -
                                           (state->player.transform.rect.x + state->player.transform.rect.width / 2),
                                       state->pickups[i].transform.rect.y -
                                           (state->player.transform.rect.y + state->player.transform.rect.height / 2)});

        Vector2 arrow_end = {
            (state->player.transform.rect.x + state->player.transform.rect.width / 2) + direction.x * arrow_length,
            (state->player.transform.rect.y + state->player.transform.rect.height / 2) + direction.y * arrow_length};

        DrawLineEx((Vector2){state->player.transform.rect.x + state->player.transform.rect.width / 2,
                             state->player.transform.rect.y + state->player.transform.rect.height / 2},
                   arrow_end, arrow_thickness, arrow_color);
    }
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

void handle_screen_select_button_upgrade(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->screen_type = IST_PLAYER_UPGRADE;
    }
}

void handle_screen_select_button_upgrade_weapons(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->screen_type = IST_PLAYER_WEAPONS_UPGRADE;
    }
}
void handle_speed_upgrade_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->player.state.coins >= state->speed_cost) {
            state->player.state.movement_speed += 10.0;
            state->player.state.coins -= state->speed_cost;
            state->speed_cost *= 1.1;
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
        game_state_start_new_wave(state);
        state->wave_strength *= 1.1;
        state->wave_number++;
        state->current_wave = generate_wave(state->wave_strength, &state->stage);
        state->player.transform.rect.x = state->stage.spawn.x;
        state->player.transform.rect.y = state->stage.spawn.y;
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

void handle_pistol_fire_rate_upgrade(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->player.state.coins > state->player.weapons[WT_PISTOL].fire_rate_upgrade_cost) {
            state->player.state.coins -= state->player.weapons[WT_PISTOL].fire_rate_upgrade_cost;
            state->player.weapons[WT_PISTOL].fire_rate -= 0.05;
            state->player.weapons[WT_PISTOL].fire_rate_upgrade_cost += 0.5;
        }
    }
}
void handle_pistol_damage_upgrade(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->player.state.coins > state->player.weapons[WT_PISTOL].damage_upgrade_cost) {
            state->player.state.coins -= state->player.weapons[WT_PISTOL].damage_upgrade_cost;
            state->player.weapons[WT_PISTOL].damage += 1;
            state->player.weapons[WT_PISTOL].damage_upgrade_cost += 0.5;
        }
    }
}
void handle_ar_fire_rate_upgrade(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->player.state.coins > state->player.weapons[WT_AR].fire_rate_upgrade_cost) {
            state->player.state.coins -= state->player.weapons[WT_AR].fire_rate_upgrade_cost;
            state->player.weapons[WT_AR].fire_rate -= 0.05;
            state->player.weapons[WT_AR].fire_rate_upgrade_cost += 0.5;
        }
    }
}
void handle_ar_damage_upgrade(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->player.state.coins > state->player.weapons[WT_AR].damage_upgrade_cost) {
            state->player.state.coins -= state->player.weapons[WT_AR].damage_upgrade_cost;
            state->player.weapons[WT_AR].damage += 1;
            state->player.weapons[WT_AR].damage_upgrade_cost += 0.5;
        }
    }
}

Clay_Color button_color(bool activecond) {
    return activecond ? (Clay_Color){120, 120, 120, 255} : (Clay_Color){90, 90, 90, 200};
}

Clay_RenderCommandArray game_state_draw_ui(const GameState *state) {
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
    Clay_BeginLayout();
    ui_container(CLAY_ID("OuterContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), 0, 0) {
        switch (state->phase) {
        case GP_MAIN: {
            ui_container(CLAY_ID("PlayerInfoContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_PERCENT(0.4),
                         CLAY_SIZING_PERCENT(0.4), 16, 0) {
                ui_label(TextFormat("Health: %d", state->player.state.health), 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                ui_label(TextFormat("Coins: %.2f", state->player.state.coins), 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                ui_label(TextFormat("Wave #%d", state->wave_number), 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                switch (state->player.selected) {
                case WT_PISTOL: {
                    ui_label("Pistol", 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                    ui_label("AR", 48, GRAY, CLAY_TEXT_ALIGN_LEFT);
                    break;
                }
                case WT_AR: {
                    ui_label("Pistol", 48, GRAY, CLAY_TEXT_ALIGN_LEFT);
                    ui_label("AR", 48, WHITE, CLAY_TEXT_ALIGN_LEFT);
                    break;
                }
                }
            }
            if (wave_is_done(&state->current_wave)) {
                CENTERED_ELEMENT(
                    ui_label("Press enter to enter the intermission screen", 36, WHITE, CLAY_TEXT_ALIGN_CENTER));
            }
            break;
        }
        case GP_STARTMENU: {
            ui_container(CLAY_ID("StartMenuContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0),
                         16, 16) {
                switch (state->main_menu_type) {
                case MMT_START: {
                    ui_container(CLAY_ID("TitleTextContainer"), CLAY_LEFT_TO_RIGHT, CLAY_SIZING_GROW(0),
                                 CLAY_SIZING_GROW(0), 0, 0) {
                        ui_label("Persona", 48, WHITE, CLAY_TEXT_ALIGN_CENTER);
                    };

                    ui_container(CLAY_ID("TitleScreenButtonContainer"), CLAY_LEFT_TO_RIGHT, CLAY_SIZING_GROW(0),
                                 CLAY_SIZING_GROW(0), 16, 16) {
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.5), "New", "NewButton",
                                       handle_begin_game_button, false);
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.5), "Continue", "ContinueButton",
                                       handle_continue_game_button, false);
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.5), "Controls", "ShowControlsButton",
                                       handle_show_controls_button, false);
                    }
                    break;
                }
                case MMT_CONTROLS: {
                    CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_PERCENT(0.25), CLAY_SIZING_GROW(0), "Go back",
                                                    "GoBackButton", handle_show_main_button, false));
                    ui_container(CLAY_ID("ControlsListingContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_GROW(0),
                                 CLAY_SIZING_GROW(0), 16, 16) {
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
                    ui_container(CLAY_ID("StageSelectContainer"), CLAY_LEFT_TO_RIGHT, CLAY_SIZING_GROW(0),
                                 CLAY_SIZING_GROW(0), 0, 0) {
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
            ui_container(CLAY_ID("IntermissionScreenContainer"), CLAY_LEFT_TO_RIGHT, CLAY_SIZING_GROW(0),
                         CLAY_SIZING_PERCENT(0.5), 16, 16) {
                ui_container(CLAY_ID("ScreenSelectButtonContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_PERCENT(0.1),
                             CLAY_SIZING_GROW(0), 0, 16) {
                    LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Upgrades", "UpgradeSelectScreenButton",
                                   handle_screen_select_button_upgrade, state->screen_type == IST_PLAYER_UPGRADE);
                    LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Weapon Upgrades",
                                   "WeaponUpgradeSelectScreenButton", handle_screen_select_button_upgrade_weapons,
                                   state->screen_type == IST_PLAYER_WEAPONS_UPGRADE);
                }
                switch (state->screen_type) {
                case IST_PLAYER_UPGRADE: {
                    LABELED_BUTTON(CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_GROW(0),
                                   TextFormat("Speed [Cost: %.2f]", state->speed_cost), "SpeedUpgradeButton",
                                   handle_speed_upgrade_button, false);
                    break;
                }
                case IST_PLAYER_WEAPONS_UPGRADE: {
                    CLAY({.id = CLAY_ID("WeaponUpgradeContainer"),
                          .layout = {
                              .sizing = {.width = CLAY_SIZING_PERCENT(0.2), .height = CLAY_SIZING_GROW(0)},
                              .layoutDirection = CLAY_TOP_TO_BOTTOM,
                              .childGap = 16,
                          }}) {
                        CLAY({.id = CLAY_ID("PistolUpgrades"),
                              .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                         .childGap = 8,
                                         .padding = {16, 16, 16, 16}},
                              .backgroundColor = {100, 100, 100, 255},
                              .cornerRadius = {16, 16, 16, 16}}) {
                            ui_label("Pistol", 48, WHITE, CLAY_TEXT_ALIGN_CENTER);
                            LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0),
                                           TextFormat("Firerate: %.2f / sec. [Cost: %.2f]",
                                                      state->player.weapons[WT_PISTOL].fire_rate / 1.0,
                                                      state->player.weapons[WT_PISTOL].fire_rate_upgrade_cost),
                                           "PistolFireRateUpgradeButton", handle_pistol_fire_rate_upgrade, false);
                            LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0),
                                           TextFormat("Damage: %d [Cost: %.2f]",
                                                      state->player.weapons[WT_PISTOL].damage,
                                                      state->player.weapons[WT_PISTOL].damage_upgrade_cost),
                                           "PistolDamageUpgradeButton", handle_pistol_damage_upgrade, false);
                        }
                        CLAY({.id = CLAY_ID("ARUpgrades"),
                              .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                         .childGap = 8,
                                         .padding = {16, 16, 16, 16}},
                              .backgroundColor = {100, 100, 100, 255},
                              .cornerRadius = {16, 16, 16, 16}}) {
                            ui_label("AR", 48, WHITE, CLAY_TEXT_ALIGN_CENTER);
                            LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0),
                                           TextFormat("Firerate: %.2f / sec. [Cost: %.2f]",
                                                      state->player.weapons[WT_AR].fire_rate / 1.0,
                                                      state->player.weapons[WT_AR].fire_rate_upgrade_cost),
                                           "ARFireRateUpgradeButton", handle_ar_fire_rate_upgrade, false);
                            LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0),
                                           TextFormat("Damage: %d [Cost: %.2f]", state->player.weapons[WT_AR].damage,
                                                      state->player.weapons[WT_AR].damage_upgrade_cost),
                                           "ARDamageUpgradeButton", handle_ar_damage_upgrade, false);
                        }
                    }
                    break;
                }
                }
            }
            break;
        }
        case GP_PAUSED: {
            ui_container(CLAY_ID("PauseMenuContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0),
                         16, 16) {
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
    }

    return Clay_EndLayout();
}

void apply_shader(RenderTexture2D *in, RenderTexture2D *out, Shader *shader) {
    BeginTextureMode(*out);
    ClearBackground(BLACK);
    if (shader != NULL) {
        BeginShaderMode(*shader);
        const float w = GetMonitorWidth(0);
        const float h = GetMonitorHeight(0);
        SetShaderValue(*shader, GetShaderLocation(*shader, "renderWidth"), &w, SHADER_UNIFORM_FLOAT);
        SetShaderValue(*shader, GetShaderLocation(*shader, "renderHeight"), &h, SHADER_UNIFORM_FLOAT);
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

void draw_centered_text(const char *message, const Font *font, size_t size, Color color, float y) {
    const Vector2 text_size = MeasureTextEx(*font, message, size, 0);
    DrawTextEx(*font, message, (Vector2){screen_centered_position(text_size.x), y}, size, 0, color);
}

void flash_error(GameState *state, char *message) {
    state->error_opacity = 1.0;
    state->error = message;
}

Stage stage_1() {
    return (Stage){.spawn = (Vector2){420.00, 306.67},
                   .platforms = {(Rectangle){.x = 3.33, .y = 665.00, .width = 1214.00, .height = 24.00},
                                 (Rectangle){.x = 1.67, .y = 3.33, .width = 24.00, .height = 674.00},
                                 (Rectangle){.x = 1183.33, .y = 3.33, .width = 34.00, .height = 684.00}},
                   .count = 3,
                   .spawns = {(Rectangle){.x = 38.34, .y = 10.00, .width = 194.00, .height = 214.00}},
                   .count_sp = 1};
}
Stage stage_2() {
    return (Stage){.spawn = (Vector2){1726.33, 912.89},
                   .platforms =
                       {
                           (Rectangle){.x = 5.33, .y = 859.33, .width = 1514.00, .height = 44.00},
                           (Rectangle){.x = 1942.33, .y = 855.00, .width = 1644.00, .height = 54.00},
                           (Rectangle){.x = 1322.22, .y = 879.11, .width = 294.00, .height = 24.00},
                           (Rectangle){.x = 1849.17, .y = 885.83, .width = 394.00, .height = 24.00},
                           (Rectangle){.x = 2.50, .y = -2.00, .width = 64.00, .height = 904.00},
                           (Rectangle){.x = 3522.50, .y = 0.00, .width = 64.00, .height = 904.00},
                           (Rectangle){.x = 1461.43, .y = 628.57, .width = 554.00, .height = 34.00},
                           (Rectangle){.x = 36.00, .y = 752.00, .width = 264.00, .height = 114.00},
                           (Rectangle){.x = 3298.00, .y = 722.00, .width = 264.00, .height = 164.00},
                           (Rectangle){.x = 1510.13, .y = 1041.56, .width = 464.00, .height = 24.00},
                           (Rectangle){.x = 1510.00, .y = 896.00, .width = 54.00, .height = 154.00},
                           (Rectangle){.x = 1922.79, .y = 891.07, .width = 54.00, .height = 174.00},
                           (Rectangle){.x = 2.00, .y = -33.50, .width = 3584.00, .height = 44.00},
                       },
                   .count = 13,
                   .spawns = {(Rectangle){.x = 79.00, .y = 495.00, .width = 224.00, .height = 144.00},
                              (Rectangle){.x = 3304.50, .y = 507.00, .width = 204.00, .height = 84.00}},
                   .count_sp = 2};
}
Stage stage_3() {
    return (Stage){
        .count = 3,
        .platforms =
            {
                (Platform){.x = 0, .y = 550, .width = 800, .height = 32},
                (Platform){.x = 100, .y = 400, .width = 600, .height = 32},
                (Platform){.x = 200, .y = 250, .width = 400, .height = 32},
            },
    };
}

#define SLOW_STRONG_ENEMY(x, y) ecs_basic_enemy((Vector2){(x), (y)}, (Vector2){64, 64}, 20, 50)
#define RANGER(x, y) ecs_ranger_enemy((Vector2){(x), (y)}, (Vector2){32, 96}, 20, 20, 3)

EnemyWave generate_wave(double strength, const Stage *stage) {
    EnemyWave wave = NULL;

    while (strength > 0) {
        size_t enemy_type = GetRandomValue(0, 1);
        size_t which_area = GetRandomValue(0, stage->count_sp - 1);
        Vector2 pos = (Vector2){
            GetRandomValue(stage->spawns[which_area].x, stage->spawns[which_area].x + stage->spawns[which_area].width),
            GetRandomValue(stage->spawns[which_area].y, stage->spawns[which_area].y + stage->spawns[which_area].height),
        };
        switch (enemy_type) {
        case 0: {
            stbds_arrput(wave, SLOW_STRONG_ENEMY(pos.x, pos.y));
            strength -= 1;
            break;
        }
        case 1: {
            stbds_arrput(wave, RANGER(pos.x, pos.y));
            strength -= 1;
            break;
        }
        }
    }
    return wave;
}

double screen_centered_position(double w) {
    return (GetMonitorWidth(0) / 2.0) - (w / 2.0);
}
