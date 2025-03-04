#ifndef WAVE_H
#define WAVE_H

#include <stddef.h>
#define MAX_WAVE_POPULATION 50

#include "enemy.h"
typedef struct {
    ECSEnemy enemies[50];
    size_t count;
} EnemyWave;

#endif
