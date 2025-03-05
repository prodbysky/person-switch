#ifndef PLAYER_H
#define PLAYER_H

#include <stddef.h>
#include <stdint.h>
#include "ecs.h"
#include "wave.h"

#define PLAYER_CLASS_SWITCH_COOLDOWN 20.0


typedef enum {
    BD_LEFT,
    BD_RIGHT,
} BulletDirection;

typedef struct {
    TransformComp transform;
    BulletDirection dir;
    double creation_time;
} ECSPlayerBullet;

typedef struct {
    ECSPlayerBullet bullets[100];
    size_t current;
} Bullets;

void bullets_spawn_bullet_system(const TransformComp* player_transform, Bullets *bullets, BulletDirection dir);
void bullets_update_system(Bullets *bullets, float dt);
void bullets_draw_system(const Bullets *bullets);

typedef struct {
    uint16_t speed_x;
    uint16_t jump_power;
} PlayerStat;

typedef enum {
    PS_TANK = 0,
    PS_MOVE = 1,
    PS_DAMAGE = 2,
    PS_COUNT
} PlayerClass;

typedef struct  {
    PlayerClass current_class;
    double last_switched;
    double last_hit;
    double last_shot;
    size_t health;
    bool dead;
} PlayerStateComp;

typedef struct {
    TransformComp transform;
    PhysicsComp physics;
    PlayerStateComp state;
} ECSPlayer;

ECSPlayer ecs_player_new();
void player_input_system(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform,
                         Bullets *bullets);
void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets);
void player_enemy_interaction_system(ECSPlayer *player, const EnemyWave *wave);
#endif
