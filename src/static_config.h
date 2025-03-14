#ifndef STATIC_CONFIG_H
#define STATIC_CONFIG_H

#ifdef RELEASE
#define WINDOW_W 800
#define WINDOW_H 600
#else
#define WINDOW_W 1200
#define WINDOW_H 600
#endif

#define G 500
#define INVULNERABILITY_TIME 0.5

#include "player.h"

static const PlayerStat PLAYER_STATES[] = {
    [PS_TANK] = {.jump_power = 300, .speed_x = 200, .damage = 2},
    [PS_MOVE] = {.jump_power = 400, .speed_x = 400, .damage = 3},
    [PS_DAMAGE] = {.jump_power = 400, .speed_x = 250, .damage = 5},
};

#endif
