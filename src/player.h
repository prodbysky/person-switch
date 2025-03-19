#ifndef PLAYER_H
#define PLAYER_H

#include <stddef.h>
#include <stdint.h>
#include "ecs.h"
#include "particles.h"
#include "pickup.h"
#include "raylib.h"
#include "wave.h"
#include "bullet.h"

#define SHOOT_DELAY 0.25

// The player stats that are dependant on the selected class
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

typedef struct {
    PlayerClass current_class;
    double last_hit;
    double last_shot;
    double last_healed;
    size_t health;
    bool dead;
    float reload_time; 
    float movement_speed;
    float coins;
} PlayerStateComp;

typedef struct {
    TransformComp transform;
    PhysicsComp physics;
    PlayerStateComp state;
    SolidRectangleComp draw_conf;
} ECSPlayer;

ECSPlayer ecs_player_new();
// Handles player input
void player_input(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform, Bullets *bullets,
                  const Sound *jump_sound, const Sound *shoot_sound, const Camera2D *camera, Particles *particles);
// Updates the entire player state
void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets,
                       const Sound *jump_sound, const Sound *shoot_sound, Bullets *enemy_bullets, Pickups *pickups, const Camera2D* camera, Particles* particles);
void player_enemy_interaction(ECSPlayer *player, const EnemyWave *wave, Bullets *enemy_bullets, Particles *particles);
void player_pickup_interaction(ECSPlayer *player, Pickups* pickups);
void player_draw(const ECSPlayer *player);
#endif
