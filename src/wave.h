#ifndef WAVE_H
#define WAVE_H

#include <stddef.h>

#include "enemy.h"

typedef ECSEnemy* EnemyWave;

bool wave_is_done(const EnemyWave* wave);
void wave_draw(const EnemyWave* wave);

#endif
