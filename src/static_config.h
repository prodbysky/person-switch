#ifndef STATIC_CONFIG_H
#define STATIC_CONFIG_H

#define WINDOW_W 800
#define WINDOW_H 600

#define G 500
#define INVULNERABILITY_TIME 0.5

#include "player.h"
static const PlayerStat PLAYER_STATES[] = {
    [PS_TANK] = {.jump_power = 150, .speed_x = 200, .damage = 2},
    [PS_MOVE] = {.jump_power = 300, .speed_x = 350, .damage = 3},
    [PS_DAMAGE] = {.jump_power = 200, .speed_x = 250, .damage = 5},
};

#endif
