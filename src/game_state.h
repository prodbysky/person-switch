#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "arena.h"
#include "player.h"
#include "wave.h"

Stage default_stage();
EnemyWave default_wave();

typedef enum {
    GP_STARTMENU = 0,
    GP_MAIN,
    GP_DEAD,
    GP_PAUSED,
    GP_AFTER_WAVE,
    GP_TRANSITION
} GamePhase;

// The entire game state, sort of a `god` object
typedef struct {
    // Internal arena allocator which will store all internal allocations for the game
    Arena allocator;

    ECSPlayer player;
    Stage stage;
    EnemyWave current_wave;
    bool wave_finished;
    GamePhase phase;
    Bullets bullets;

    // Phase Transitions
    double began_transition;
    GamePhase after_transition;

    // Assets
    Font font;
    Sound player_jump_sound;
    Sound player_shoot_sound;
    Sound enemy_hit_sound;
    Sound phase_change_sound;
} GameState;


GameState game_state_init(); 
void game_state_destroy(GameState *state); 
void game_state(GameState *state); 
void game_state_update(GameState *state);
void game_state_frame(const GameState *state); 
#ifndef RELEASE
void game_state_draw_debug_stats(const GameState *state);
#endif
void game_state_draw_playfield(const GameState *state); 
#endif
