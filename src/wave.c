#include "wave.h"
#include "enemy.h"
#include <stb_ds.h>
#include <stddef.h>

bool wave_is_done(const EnemyWave *wave) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*wave); i++) {
        if (!(*wave)[i].state.dead) {
            return false;
        }
    }
    return true;
}

void wave_draw(const EnemyWave *wave) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*wave); i++) {
        const ECSEnemy *enemy = &(*wave)[i];
        if (!enemy->state.dead) {
            enemy_draw_self(enemy);
            enemy_draw_health_bar(enemy);
        }
    }
}
