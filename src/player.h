#ifndef PLAYER_H
#define PLAYER_H

#include <stddef.h>
#include <stdint.h>
#include "ecs.h"
#include "wave.h"
#include "bullet.h"

typedef struct {
    uint16_t speed_x;
    uint16_t jump_power;
    uint16_t damage;
} PlayerStat;

typedef enum {
    PS_TANK = 0,
    PS_MOVE = 1,
    PS_DAMAGE = 2,
    PS_COUNT
} PlayerClass;

typedef struct  {
    PlayerClass current_class;
    double last_hit;
    double last_shot;
    size_t health;
    bool dead;
} PlayerStateComp;

typedef struct {
    TransformComp transform;
    PhysicsComp physics;
    PlayerStateComp state;
    Color c;
} ECSPlayer;

ECSPlayer ecs_player_new();
void player_input(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform, Bullets *bullets,
                  const Sound *jump_sound, const Sound *shoot_sound);
void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets, 
                  const Sound *jump_sound, const Sound* shoot_sound);
void player_enemy_interaction(ECSPlayer *player, const EnemyWave *wave);
#endif
