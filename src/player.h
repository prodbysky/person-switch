#ifndef PLAYER_H
#define PLAYER_H

#include <stddef.h>
#include <stdint.h>
#include <raylib.h>
#include "weapon.h"
#include "ecs.h"
#include "particles.h"
#include "pickup.h"
#include "wave.h"
#include "bullet.h"

#define SHOOT_DELAY 0.25


typedef enum {
    PS_TANK = 0,
    PS_MOVE = 1,
    PS_DAMAGE = 2,
    PS_COUNT
} PlayerClass;

typedef struct {
    PlayerClass current_class;
    double last_hit;
    double last_shot;
    double last_healed;
    bool dead;
    double jump_power;
    float reload_time; 
    float movement_speed;
    float coins;
} PlayerStateComp;

typedef struct {
    TransformComp transform;
    PhysicsComp physics;
    PlayerStateComp state;
    HealthComp health;
    SolidRectangleComp draw_conf;
    size_t selected;
    Weapon weapons[WT_COUNT];
    Sound jump_sound;
} ECSPlayer;


ECSPlayer ecs_player_new();
// Handles player input
void player_input(ECSPlayer *player, Bullets *bullets,
                  const Camera2D *camera, Particles *particles);
// Updates the entire player state
void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets,
                       Bullets *enemy_bullets, Pickups *pickups, const Camera2D* camera, Particles* particles);
void player_enemy_interaction(ECSPlayer *player, const EnemyWave *wave, Bullets *enemy_bullets, Particles *particles);
void player_pickup_interaction(ECSPlayer *player, Pickups* pickups);
void player_draw(const ECSPlayer *player);
#endif
