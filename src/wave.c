#include "wave.h"
bool wave_is_done(const EnemyWave *wave) {
    for (size_t i = 0; i < wave->count; i++) {
        if (!wave->enemies[i].state.dead) {
            return false;
        }
    }
    return true;
}
