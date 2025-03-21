#include "wave.h"
#include "enemy.h"
#include "raylib.h"
bool wave_is_done(const EnemyWave *wave) {
    for (size_t i = 0; i < wave->count; i++) {
        if (!wave->enemies[i].state.dead) {
            return false;
        }
    }
    return true;
}

void wave_draw(const EnemyWave *wave) {
    for (size_t i = 0; i < wave->count; i++) {
        const ECSEnemy *enemy = &wave->enemies[i];
        if (!enemy->state.dead) {
            DrawRectangleRec(
                (Rectangle){
                    .x = enemy->transform.rect.x - enemy->transform.rect.width / 2.0,
                    .y = enemy->transform.rect.y - 32,
                    .width = enemy->transform.rect.width * 2,
                    .height = 16,
                },
                GetColor(0x990000ff));
            DrawRectangleRec(
                (Rectangle){
                    .x = enemy->transform.rect.x - enemy->transform.rect.width / 2.0,
                    .y = enemy->transform.rect.y - 32,
                    .width = ((enemy->transform.rect.width * 2) / (float)enemy->state.health.max) *
                             enemy->state.health.current,
                    .height = 16,
                },
                RED);
            draw_solid(&enemy->transform, &enemy->draw_conf);
        }
    }
}
