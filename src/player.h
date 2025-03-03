#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <raylib.h>
#include "stage.h"
#include "ecs.h"

#define PLAYER_CLASS_SWITCH_COOLDOWN 20.0

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
} PlayerStateComp;

typedef struct {
    TransformComp transform;
    ColliderComp collider;
    PhysicsComp physics;
    PlayerStateComp state;
} ECSPlayer;

ECSPlayer ecs_player_new();
void player_input_system(PlayerStateComp* state, PhysicsComp* physics, const ColliderComp* collider); 
void ecs_player_update(ECSPlayer* player, const Stage* stage);
#endif
