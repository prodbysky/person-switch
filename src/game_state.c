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
#include <stdio.h>
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

GameState game_state_init() {
    GameState st = {0};
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
    st.stages = load_stages("assets/stages/index.sti", "assets/stages/stage%zu.st");

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
    case GP_EDITOR: {
        DrawText("0, 0", 10, 10, 36, WHITE);
        DrawLineEx((Vector2){0, -3000}, (Vector2){0, 3000}, 5, GRAY);
        DrawLineEx((Vector2){-3000, 0}, (Vector2){3000, 0}, 5, GRAY);
        for (size_t i = 0; i < state->editor_state.s.count; i++) {
            DrawRectangleRec(state->editor_state.s.platforms[i], WHITE);
            if (i == state->editor_state.selected) {
                DrawRectangleLinesEx(state->editor_state.s.platforms[i], 2, RED);
            }
        }
        for (size_t i = 0; i < state->editor_state.s.count_sp; i++) {
            DrawRectangleRec(state->editor_state.s.spawns[i], GetColor(0x00ff0066));
            if (i + 50 == state->editor_state.selected) {
                DrawRectangleLinesEx(state->editor_state.s.spawns[i], 2, RED);
            }
        }
        DrawCircleV(state->editor_state.s.spawn, 25, GREEN);
        DrawRectangle(state->editor_state.s.spawn.x, state->editor_state.s.spawn.y, 32, 96, WHITE);
        if (state->editor_state.selected == SPAWN_SELECT) {
            DrawCircleLinesV(state->editor_state.s.spawn, 25, RED);
        }
        break;
    }
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
        if (state->player.health.current < 5) {
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
        game_state_update_camera(&state->camera, &state->player.transform);
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
    case GP_EDITOR:
        game_state_update_editor(state, dt);
        break;
    }
}

void game_state_destroy(GameState *state) {
    UnloadFont(state->font[0]);
    UnloadRenderTexture(state->raw_frame_buffer);
    UnloadRenderTexture(state->ui_frame_buffer);
    UnloadRenderTexture(state->final_frame_buffer);
    UnloadShader(state->pixelizer);
    stbds_arrfree(state->bullets);
    stbds_arrfree(state->enemy_bullets);
    stbds_arrfree(state->current_wave);
    stbds_arrfree(state->pickups);
    stbds_arrfree(state->particles);
    save_stages(&state->stages, "assets/stages/index.sti", "assets/stages/stage%zu.st");
    stbds_arrfree(state->stages);
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
                         &state->pickups, &state->particles, state->current_wave, stbds_arrlen(state->current_wave));
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

            state->wave_strength *= 1.2;
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
        state->stage = state->stages[state->selected_stage];
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

void game_state_update_editor(GameState *state, float dt) {
    (void)dt;
    const Vector2 scroll_delta = GetMouseWheelMoveV();
    const Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const Vector2 mouse_delta = GetMouseDelta();
        state->camera.target =
            Vector2Subtract(state->camera.target, Vector2Scale(mouse_delta, 1.0 / state->camera.zoom));
    }

    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (scroll_delta.y != 0) {
            state->camera.zoom += scroll_delta.y * 0.1;
        }
    }
    state->camera.zoom = Clamp(state->camera.zoom, 0.2, 4);

    Vector2 world_mouse_pos = GetScreenToWorld2D(mouse_pos, state->camera);
    if (CheckCollisionPointCircle(world_mouse_pos, state->editor_state.s.spawn, 25)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->editor_state.selected = SPAWN_SELECT;
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse_delta = GetMouseDelta();
            if (state->editor_state.selected == SPAWN_SELECT) {
                state->editor_state.s.spawn.x += mouse_delta.x / state->camera.zoom;
                state->editor_state.s.spawn.y += mouse_delta.y / state->camera.zoom;
            }
        }
    } else {
        for (size_t i = 0; i < state->editor_state.s.count; i++) {
            if (CheckCollisionPointRec(world_mouse_pos, state->editor_state.s.platforms[i])) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state->editor_state.selected = i;
                }
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    const Vector2 mouse_delta = GetMouseDelta();
                    if (state->editor_state.selected != 0xffff && state->editor_state.selected != SPAWN_SELECT) {
                        state->editor_state.s.platforms[state->editor_state.selected].x +=
                            mouse_delta.x / state->camera.zoom;
                        state->editor_state.s.platforms[state->editor_state.selected].y +=
                            mouse_delta.y / state->camera.zoom;
                    }
                }
                if (state->editor_state.selected != 0xffff && state->editor_state.selected != SPAWN_SELECT) {
                    if (IsKeyDown(KEY_LEFT_SHIFT)) {
                        if (scroll_delta.y != 0) {
                            state->editor_state.s.platforms[state->editor_state.selected].width += scroll_delta.y * 10;
                        }
                    }
                    if (IsKeyDown(KEY_LEFT_ALT)) {
                        if (scroll_delta.y != 0) {
                            state->editor_state.s.platforms[state->editor_state.selected].height += scroll_delta.y * 10;
                        }
                    }
                }
            }
        }
        for (size_t i = 0; i < state->editor_state.s.count_sp; i++) {
            if (CheckCollisionPointRec(world_mouse_pos, state->editor_state.s.spawns[i])) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state->editor_state.selected = i + 50;
                }
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    const Vector2 mouse_delta = GetMouseDelta();
                    if (state->editor_state.selected != 0xffff && state->editor_state.selected != SPAWN_SELECT &&
                        state->editor_state.selected > 49) {
                        state->editor_state.s.spawns[state->editor_state.selected - 50].x +=
                            mouse_delta.x / state->camera.zoom;
                        state->editor_state.s.spawns[state->editor_state.selected - 50].y +=
                            mouse_delta.y / state->camera.zoom;
                    }
                }
                if (state->editor_state.selected != 0xffff && state->editor_state.selected != SPAWN_SELECT &&
                    state->editor_state.selected > 49) {
                    if (IsKeyDown(KEY_LEFT_SHIFT)) {
                        if (scroll_delta.y != 0) {
                            state->editor_state.s.spawns[state->editor_state.selected - 50].width +=
                                scroll_delta.y * 10;
                        }
                    }
                    if (IsKeyDown(KEY_LEFT_ALT)) {
                        if (scroll_delta.y != 0) {
                            state->editor_state.s.spawns[state->editor_state.selected - 50].height +=
                                scroll_delta.y * 10;
                        }
                    }
                }
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        state->editor_state.selected = 0xffff;
    }
    if (state->editor_state.selected == 0xffff || state->editor_state.selected == SPAWN_SELECT)
        return;
    float moveStep = 5.0f * GetFrameTime();
    Rectangle *obj = NULL;
    if (state->editor_state.selected < state->editor_state.s.count) {
        obj = &state->editor_state.s.platforms[state->editor_state.selected];
    } else if (state->editor_state.selected >= 50 &&
               state->editor_state.selected < 50 + state->editor_state.s.count_sp) {
        obj = &state->editor_state.s.spawns[state->editor_state.selected - 50];
    }
    if (obj) {
        if (IsKeyDown(KEY_RIGHT))
            obj->x += moveStep;
        if (IsKeyDown(KEY_LEFT))
            obj->x -= moveStep;
        if (IsKeyDown(KEY_DOWN))
            obj->y += moveStep;
        if (IsKeyDown(KEY_UP))
            obj->y -= moveStep;
    }
}

void game_state_phase_change(GameState *state, GamePhase next) {
    if (next == GP_MAIN) {
        state->stage = state->stages[state->selected_stage];
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
        state->stage = state->stages[state->selected_stage];
        game_state_start_new_wave(state);
        state->wave_strength *= 1.1;
        state->wave_number++;
        state->current_wave = generate_wave(state->wave_strength, &state->stage);
        state->player.transform.rect.x = state->stage.spawn.x;
        state->player.transform.rect.y = state->stage.spawn.y;
    }
}

void handle_editor_start_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        game_state_phase_change(state, GP_EDITOR);
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

void handle_exit_editor(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        game_state_phase_change(state, GP_STARTMENU);
        state->camera = (Camera2D){
            .zoom = 0.75,
            .offset = (Vector2){GetMonitorWidth(0) / 2.0, GetMonitorHeight(0) / 2.0},
            .rotation = 0,
            .target = (Vector2){GetMonitorWidth(0) / 2.0, GetMonitorHeight(0) / 2.0},
        };
    }
}

void handle_save_stage_editor(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    GameState *state = (GameState *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        stbds_arrput(state->stages, state->editor_state.s);
    }
}

void handle_editor_add_platform(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    GameState *state = (GameState *)ud;
    (void)e_id;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->editor_state.s.platforms[state->editor_state.s.count++] = (Rectangle){
            .x = 0,
            .y = 0,
            .width = 64,
            .height = 64,
        };
    }
}
void handle_editor_add_spawn(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    GameState *state = (GameState *)ud;
    (void)state;
    (void)e_id;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->editor_state.s.spawns[state->editor_state.s.count_sp++] = (Rectangle){
            .x = 0,
            .y = 0,
            .width = 64,
            .height = 64,
        };
    }
}

void handle_editor_delete_object(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    GameState *state = (GameState *)ud;
    (void)state;
    (void)e_id;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->editor_state.selected == SPAWN_SELECT) {
            state->editor_state.s.spawn = (Vector2){0, 0};
        } else if (state->editor_state.selected < state->editor_state.s.count) {
            for (size_t i = state->editor_state.selected; i < state->editor_state.s.count - 1; i++) {
                state->editor_state.s.platforms[i] = state->editor_state.s.platforms[i + 1];
            }
            state->editor_state.s.count--;
            state->editor_state.selected = 0xffff;
        } else if (state->editor_state.selected >= 50 &&
                   state->editor_state.selected < 50 + state->editor_state.s.count_sp) {
            size_t idx = state->editor_state.selected - 50;
            for (size_t i = idx; i < state->editor_state.s.count_sp - 1; i++) {
                state->editor_state.s.spawns[i] = state->editor_state.s.spawns[i + 1];
            }
            state->editor_state.s.count_sp--;
            state->editor_state.selected = 0xffff;
        }
    }
}
void handle_editor_copy_object(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    GameState *state = (GameState *)ud;
    (void)state;
    (void)e_id;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (state->editor_state.selected == SPAWN_SELECT)
            return;
        else if (state->editor_state.selected < state->editor_state.s.count) {
            state->editor_state.copied_rect = state->editor_state.s.platforms[state->editor_state.selected];
            state->editor_state.copied_is_platform = true;
            state->editor_state.has_copied = true;
        } else if (state->editor_state.selected >= 50 &&
                   state->editor_state.selected < 50 + state->editor_state.s.count_sp) {
            state->editor_state.copied_rect = state->editor_state.s.spawns[state->editor_state.selected - 50];
            state->editor_state.copied_is_platform = false;
            state->editor_state.has_copied = true;
        }
    }
}
void handle_editor_paste_object(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    GameState *state = (GameState *)ud;
    (void)state;
    (void)e_id;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        Vector2 mouse_pos = GetMousePosition();
        Vector2 world_mouse = GetScreenToWorld2D(mouse_pos, state->camera);
        Rectangle new_rect = state->editor_state.copied_rect;
        new_rect.x = world_mouse.x;
        new_rect.y = world_mouse.y;
        if (state->editor_state.copied_is_platform) {
            state->editor_state.s.platforms[state->editor_state.s.count++] = new_rect;
            state->editor_state.selected = state->editor_state.s.count - 1;
        } else {
            state->editor_state.s.spawns[state->editor_state.s.count_sp++] = new_rect;
            state->editor_state.selected = 50 + state->editor_state.s.count_sp - 1;
        }
    }
}

typedef struct {
    GameState *state;
    ptrdiff_t stage_index;
} ThatOneSpecificInput;
void handle_select_stage_button(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    ThatOneSpecificInput *state = (ThatOneSpecificInput *)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->state->selected_stage = state->stage_index;
    }
}

Clay_Color button_color(bool activecond) {
    return activecond ? (Clay_Color){120, 120, 120, 255} : (Clay_Color){90, 90, 90, 200};
}
void text(Clay_String str, Clay_Color color, Clay_TextAlignment alligment) {
    CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                       .textAlignment = alligment,
                       .textColor = color,
                       .fontSize = 32,
                       .fontId = 0,
                   }));
}

void button(Clay_ElementId id, Clay_SizingAxis x_sizing, Clay_SizingAxis y_sizing, Clay_String label,
            Clay_Color background_color, Clay_Color label_color,
            void (*on_hover)(Clay_ElementId, Clay_PointerData, intptr_t), intptr_t ud) {
    CLAY({.id = id,
          .cornerRadius = {16, 16, 16, 16},
          .layout = {.sizing = {x_sizing, y_sizing},
                     .padding = {16, 16, 16, 16},
                     .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}},
          .backgroundColor = background_color}) {
        Clay_OnHover(on_hover, ud);
        text(label, label_color, CLAY_TEXT_ALIGN_CENTER);
    }
}

Clay_RenderCommandArray game_state_draw_ui(GameState *state) {
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
    ui_container(CLAY_ID("OuterContainer"), CLAY_TOP_TO_BOTTOM, CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), 0, 16) {

        switch (state->phase) {
        case GP_MAIN: {
            if (wave_is_done(&state->current_wave)) {
                CENTERED_ELEMENT(
                    ui_label("Press enter to enter the intermission screen", 36, WHITE, CLAY_TEXT_ALIGN_CENTER));
            }
            CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.9)}}});
            CLAY({
                .layout =
                    {
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                        .childGap = 16,
                        .padding = {16, 16, 16, 16},
                    },
                /*.backgroundColor = {100, 100, 100, 255}*/
            }) {
                CLAY({
                    .backgroundColor = {100, 0, 0, 255},
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                        },
                    .cornerRadius = {16, 16, 16, 16},
                }) {
                    CLAY({.backgroundColor = {255, 0, 0, 255},
                          .cornerRadius = {16, 16, 16, 16},
                          .layout = {.sizing = {
                                         CLAY_SIZING_PERCENT(state->player.health.current / state->player.health.max),
                                         CLAY_SIZING_GROW(0),
                                     }}});
                }
                CLAY({.backgroundColor = {100, 100, 100, 255},
                      .cornerRadius = {16, 16, 16, 16},
                      .layout = {.sizing =
                                     {
                                         CLAY_SIZING_GROW(0),
                                         CLAY_SIZING_GROW(0),
                                     },
                                 .childGap = 16,
                                 .padding = {16, 16, 16, 16}}}) {
                    LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "Pistol", "PistolLabel", NULL,
                                   state->player.selected == WT_PISTOL);
                    LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), "AR", "ARLabel", NULL,
                                   state->player.selected == WT_AR);
                }
                CLAY({.backgroundColor = {100, 100, 100, 255},
                      .cornerRadius = {16, 16, 16, 16},
                      .layout = {.sizing =
                                     {
                                         CLAY_SIZING_PERCENT(0.3),
                                         CLAY_SIZING_GROW(0),
                                     },
                                 .childGap = 16,
                                 .padding = {16, 16, 16, 16},
                                 .layoutDirection = CLAY_TOP_TO_BOTTOM}}) {
                    ui_label(TextFormat("Wave #%d", state->wave_number - 1), 36, WHITE, CLAY_TEXT_ALIGN_CENTER);
                    ui_label(TextFormat("Cash: %.2f", state->player.state.coins), 36, WHITE, CLAY_TEXT_ALIGN_CENTER);
                }
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
                        LABELED_BUTTON(CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.5), "Editor", "StartEditor",
                                       handle_editor_start_button, false);
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
                    ui_container(CLAY_ID("ConfigGameContainer"), CLAY_LEFT_TO_RIGHT, CLAY_SIZING_GROW(0),
                                 CLAY_SIZING_GROW(0), 0, 0) {
                        CLAY({.backgroundColor = {20, 20, 20, 255},
                              .cornerRadius = {16, 16, 16, 16},
                              .id = CLAY_ID("StageSelectContainer"),
                              .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                         .childGap = 16,
                                         .padding = {16, 16, 16, 16}},
                              .scroll = {.vertical = true}}) {
                            for (ptrdiff_t i = 0; i < stbds_arrlen(state->stages); i++) {
                                const ThatOneSpecificInput input = (ThatOneSpecificInput){state, i};
                                const char *cstr = TextFormat("Stage #%zu", i + 1);
                                const Clay_String str = {.chars = cstr, .length = strlen(cstr)};
                                button(CLAY_IDI("StageButton", i), CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.1), str,
                                       (Clay_Color){50, 50, 50, 255}, (Clay_Color){255, 255, 255, 255},
                                       handle_select_stage_button, (intptr_t)&input);
                            }
                        }

                        ui_container(CLAY_ID("ButtonContainer"), CLAY_LEFT_TO_RIGHT, CLAY_SIZING_GROW(0),
                                     CLAY_SIZING_GROW(0), 0, 0) {
                            CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_PERCENT(0.25), CLAY_SIZING_GROW(0), "Play",
                                                            "PlayButton", handle_start_game_button, false));

                            CENTERED_ELEMENT(LABELED_BUTTON(CLAY_SIZING_PERCENT(0.25), CLAY_SIZING_GROW(0), "Go back",
                                                            "GoBackButton", handle_show_main_button, false));
                        }
                    }
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
        case GP_EDITOR: {
            CLAY({.id = CLAY_ID("Toolbar"),
                  .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.1)},
                             .padding = {16, 16, 16, 16},
                             .childGap = 16},
                  .cornerRadius = {16, 16, 16, 16},
                  .backgroundColor = {50, 50, 50, 255}}) {
                button(CLAY_ID("QuitButton"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("X"),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_exit_editor,
                       (intptr_t)state);
                button(CLAY_ID("SaveStage"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("Save"),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_save_stage_editor,
                       (intptr_t)state);
            }
            CLAY({.id = CLAY_ID("ObjectControls"),
                  .cornerRadius = {16, 16, 16, 16},
                  .layout = {.sizing = {CLAY_SIZING_PERCENT(0.4), CLAY_SIZING_PERCENT(.1)},
                             .padding = {16, 16, 16, 16},
                             .childGap = 16},
                  .backgroundColor = {50, 50, 50, 255}}) {
                button(CLAY_ID("AddPlatform"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Add Plat."),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_editor_add_platform,
                       (intptr_t)state);
                button(CLAY_ID("AddSpawn"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Add Spawn."),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_editor_add_spawn,
                       (intptr_t)state);
                button(CLAY_ID("Delete"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Delete"),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_editor_delete_object,
                       (intptr_t)state);
                button(CLAY_ID("Copy"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Copy"),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_editor_copy_object,
                       (intptr_t)state);
                button(CLAY_ID("Paste"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Paste"),
                       (Clay_Color){20, 20, 20, 255}, (Clay_Color){255, 255, 255, 255}, handle_editor_paste_object,
                       (intptr_t)state);
            }
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

Stage *load_stages(const char *index_file_name, const char *stage_file_name_format) {
    FILE *index_file = fopen(index_file_name, "r");
    size_t n;
    fscanf(index_file, "%zu", &n);
    fclose(index_file);
    Stage *stages = NULL;
    for (size_t i = 0; i < n; i++) {
        FILE *stage_file = fopen(TextFormat(stage_file_name_format, i), "r");
        Stage stage;
        fscanf(stage_file, "%f %f", &stage.spawn.x, &stage.spawn.y);
        fscanf(stage_file, "%zu", &stage.count);
        for (size_t j = 0; j < stage.count; j++) {
            fscanf(stage_file, "%f %f %f %f", &stage.platforms[j].x, &stage.platforms[j].y, &stage.platforms[j].width,
                   &stage.platforms[j].height);
        }

        fscanf(stage_file, "%zu", &stage.count_sp);
        for (size_t j = 0; j < stage.count_sp; j++) {
            fscanf(stage_file, "%f %f %f %f", &stage.spawns[j].x, &stage.spawns[j].y, &stage.spawns[j].width,
                   &stage.spawns[j].height);
        }
        fclose(stage_file);
        stbds_arrput(stages, stage);
    }
    return stages;
}
void save_stages(Stage **stages, const char *index_file_name, const char *stage_file_name_format) {
    FILE *index_file = fopen(index_file_name, "w");
    fprintf(index_file, "%zu", stbds_arrlen(*stages));
    fclose(index_file);
    for (ptrdiff_t i = 0; i < stbds_arrlen(*stages); i++) {
        FILE *stage_file = fopen(TextFormat(stage_file_name_format, i), "w");

        fprintf(stage_file, "%.2f %.2f\n", (*stages)[i].spawn.x, (*stages)[i].spawn.y);
        fprintf(stage_file, "%zu\n", (*stages)[i].count);
        for (size_t j = 0; j < (*stages)[i].count; j++) {
            fprintf(stage_file, "%.2f %.2f %.2f %.2f\n", (*stages)[i].platforms[j].x, (*stages)[i].platforms[j].y,
                    (*stages)[i].platforms[j].width, (*stages)[i].platforms[j].height);
        }
        fprintf(stage_file, "%zu\n", (*stages)[i].count_sp);
        for (size_t j = 0; j < (*stages)[i].count_sp; j++) {
            fprintf(stage_file, "%.2f %.2f %.2f %.2f\n", (*stages)[i].spawns[j].x, (*stages)[i].spawns[j].y,
                    (*stages)[i].spawns[j].width, (*stages)[j].spawns[j].height);
        }
        fclose(stage_file);
    }
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

#define SLOW_STRONG_ENEMY(x, y) ecs_basic_enemy((Vector2){(x), (y)}, (Vector2){64, 64}, 10, 50)
#define RANGER(x, y) ecs_ranger_enemy((Vector2){(x), (y)}, (Vector2){32, 96}, 20, 20, 3)
#define DRONE(x, y) ecs_drone_enemy((Vector2){(x), (y)}, (Vector2){64, 32}, 10, 20, 3, 300)
#define WOLF(x, y) ecs_wolf_enemy((Vector2){(x), (y)}, (Vector2){64, 20}, 5, 20, 2000, 0, 3)
#define HEALER(x, y) ecs_healing_enemy((Vector2){(x), (y)}, (Vector2){32, 64}, 5, 9, 0.5, 200)

EnemyWave generate_wave(double strength, const Stage *stage) {
    EnemyWave wave = NULL;

    while (strength > 0) {
        size_t enemy_type = GetRandomValue(0, 4);
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
        case 2: {
            stbds_arrput(wave, DRONE(pos.x, pos.y));
            strength -= 2;
            break;
        }
        case 3: {
            stbds_arrput(wave, WOLF(pos.x, pos.y));
            strength -= 2;
            break;
        }
        case 4: {
            stbds_arrput(wave, HEALER(pos.x, pos.y));
            strength -= 2;
            break;
        }
        }
    }
    return wave;
}

double screen_centered_position(double w) {
    return (GetMonitorWidth(0) / 2.0) - (w / 2.0);
}
