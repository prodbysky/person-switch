#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "pickup.h"
#include "player.h"
#include "particles.h"
#include "raylib.h"
#include "wave.h"
#include <clay/clay.h>

Stage stage_1();
Stage stage_2();
Stage stage_3();

typedef enum {
    GP_STARTMENU = 0,
    GP_MAIN,
    GP_DEAD,
    GP_PAUSED,
    GP_AFTER_WAVE,
    GP_TRANSITION
} GamePhase;

typedef enum {
    IST_PLAYER_UPGRADE = 0
} IntermissionScreenType;

typedef enum {
    MMT_START,
    MMT_CONFIGGAME,
    MMT_CONTROLS,
} MainMenuType;


// The entire game state, sort of a `god` object
typedef struct {
    // Player state
    ECSPlayer player;
    double reload_cost;
    double speed_cost;
    Bullets bullets;

    // Contains platforms
    Stage stage;
    size_t selected_stage;

    EnemyWave current_wave;
    Bullets enemy_bullets;

    GamePhase phase;

    // Phase Transitions
    double began_transition;
    GamePhase after_transition;

    // Assets
    Font font[1];
    Sound ui_button_click_sound;
    Sound enemy_hit_sound;
    Sound enemy_die_sound;
    Sound phase_change_sound;

    // Intermission screen (upgrades)
    IntermissionScreenType screen_type;

    double wave_strength;
    size_t wave_number;

    Camera2D camera;

    double volume_label_opacity;
    double vfx_indicator_opacity;

    char* error;
    double error_opacity;

    MainMenuType main_menu_type;

    // vfx
    RenderTexture2D raw_frame_buffer;
    RenderTexture2D ui_frame_buffer;
    RenderTexture2D final_frame_buffer;
    Shader pixelizer;
    bool vfx_enabled;

    // clay ui
    Clay_Arena clay_memory;

    Pickups pickups;
    Particles particles;
} GameState;

// Initializes raylib, loads needed assets
GameState game_state_init(); 

// Closes the raylib context
void game_state_destroy(GameState *state); 

// Starts a new wave with the given new class of player
void game_state_start_new_wave(GameState *state);

// Runs a single frame (update and draw) of the game
void game_state(GameState *state); 
// Runs actual logic of the game, also plays sounds,
void game_state_update(GameState *state);
// Draws a single frame of the game state
void game_state_frame(GameState *state); 
// Draws the stage, enemies, bullets, player
void game_state_draw_playfield(const GameState *state); 
Clay_RenderCommandArray game_state_draw_ui(const GameState *state);


void game_state_update_gp_main(GameState *state, float dt);
void game_state_update_gp_dead(GameState *state, float dt);
void game_state_update_gp_paused(GameState *state, float dt);
void game_state_update_gp_transition(GameState *state, float dt);
void game_state_update_gp_after_wave(GameState *state, float dt);

void game_state_update_ui_internals();
void game_state_update_camera(Camera2D *camera, const TransformComp *target);

void game_state_phase_change(GameState *state, GamePhase next);

void draw_centered_text(const char* message, const Font* font, size_t size, Color color, float y);
double screen_centered_position(double w);

EnemyWave generate_wave(double strength, const Stage *stage);

void apply_shader(RenderTexture2D *in, RenderTexture2D *out, Shader *shader);

void ui_label(const char *text, uint16_t size, Color c, Clay_TextAlignment aligment);
void flash_error(GameState* state, char* message);
#define ui_container(id_, dir, width, height, pad, c_gap)  \
    CLAY({ \
        .id = id_, \
        .layout = { \
            .sizing = {width, height}, \
            .layoutDirection = dir, \
            .childGap = c_gap, \
            .padding = {pad, pad, pad, pad}, \
        } \
    }) 
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
#endif
