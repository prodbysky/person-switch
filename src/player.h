#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <raylib.h>

#define PLAYER_CLASS_SWITCH_COOLDOWN 20.0

typedef struct {
    uint16_t speed_x;
    uint16_t jump_power;
} PlayerStat;

typedef enum {
    PS_TANK = 0,
    PS_MOVE = 1,
    PS_DAMAGE = 2
} PlayerClass;

typedef struct {
    Rectangle rect;
    Vector2 velocity;
    PlayerClass current_class;
    double last_switched;
    bool grounded;
} Player;

Player player_new();
void player_update(Player *player);
#endif
