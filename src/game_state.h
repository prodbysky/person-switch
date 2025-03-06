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
} GamePhase;

// The entire game state, sort of a `god` object
typedef struct {
    // Internal arena allocator which will store all internal allocations for the game
    Arena allocator;
    ECSPlayer player;
    Stage stage;
    EnemyWave current_wave;
    GamePhase phase;
    Bullets bullets;
    Font font;
} GameState;


GameState game_state_init(); 
void game_state_destroy(GameState *state); 
void game_state(GameState *state); 
void game_state_update(GameState *state);
void game_state_frame(const GameState *state); 
void game_state_draw_debug_stats(const GameState *state);
void game_state_draw_playfield(const GameState *state); 
#endif
