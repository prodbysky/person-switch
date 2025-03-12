#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "player.h"
#include "raylib.h"
#include "wave.h"
#include <clay/clay.h>

Stage default_stage();

typedef enum {
    GP_STARTMENU = 0,
    GP_MAIN,
    GP_DEAD,
    GP_PAUSED,
    GP_AFTER_WAVE,
    GP_TRANSITION
} GamePhase;

typedef enum {
    IST_PLAYER_CLASS_SELECT = 0,
    IST_PLAYER_UPGRADE = 1
} IntermissionScreenType;

typedef enum {
    MMT_START,
    MMT_CONTROLS,
} MainMenuType;


// The entire game state, sort of a `god` object
typedef struct {
    // Player state
    ECSPlayer player;
    Bullets bullets;

    // Contains platforms
    Stage stage;

    EnemyWave current_wave;
    Bullets enemy_bullets;

    GamePhase phase;

    // Phase Transitions
    double began_transition;
    GamePhase after_transition;

    // Assets
    Font font[1];
    Sound player_jump_sound;
    Sound player_shoot_sound;
    Sound enemy_hit_sound;
    Sound phase_change_sound;

    // Intermission screen (upgrades)
    IntermissionScreenType screen_type;
    PlayerClass selected_class;

    double wave_strength;
    size_t wave_number;

    Camera2D camera;

    double volume_label_opacity;

    MainMenuType main_menu_type;

    // vfx
    RenderTexture2D target;
    Shader pixelizer;
    bool vfx_enabled;

    // clay ui
    Clay_Arena clay_memory;
} GameState;

// Initializes raylib, loads needed assets
GameState game_state_init(); 

// Closes the raylib context
void game_state_destroy(GameState *state); 

// Starts a new wave with the given new class of player
void game_state_start_new_wave(GameState* state, PlayerClass new_class);

// Runs a single frame (update and draw) of the game
void game_state(GameState *state); 
// Runs actual logic of the game, also plays sounds,
void game_state_update(GameState *state);
// Draws a single frame of the game state
void game_state_frame(GameState *state); 
// Draws the stage, enemies, bullets, player
void game_state_draw_playfield(const GameState *state); 
Clay_RenderCommandArray game_state_draw_ui(const GameState *state);


void draw_centered_text(const char* message, const Font* font, size_t size, Color color, float y);
double screen_centered_position(double w);

EnemyWave generate_wave(double strength);

void ui_label(const char *text, uint16_t size, Color c, Clay_TextAlignment aligment);

#endif
