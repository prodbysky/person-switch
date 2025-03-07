#ifndef WAVE_H
#define WAVE_H

#include <stddef.h>
#define MAX_WAVE_POPULATION 50

#include "enemy.h"
typedef struct {
    ECSEnemy enemies[MAX_WAVE_POPULATION];
    size_t count;
} EnemyWave;

bool wave_is_done(const EnemyWave* wave);

#endif
