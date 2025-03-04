#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "arena.h"
#include "player.h"
#include "enemy.h"

Stage default_stage(Arena *arena);

typedef struct {
    Arena allocator;
    ECSPlayer player;
    Stage stage;
    ECSEnemy test_enemy;
} GameState;

GameState game_state_init_system(); 
void game_state_destroy(GameState *state); 
void game_state_system(GameState *state); 
void game_state_update_system(GameState *state);
void game_state_frame_system(const GameState *state); 
void game_state_draw_debug_stats(const GameState *state); 
void game_state_draw_playfield_system(const GameState *state); 
#endif
