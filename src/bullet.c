#include "bullet.h"
#include "ecs.h"
#include "raylib.h"
#include "timing_utilities.h"

void bullets_spawn_bullet(const TransformComp *origin_transform, Bullets *bullets, BulletDirection dir, Color c) {
    const float x = origin_transform->rect.x + (origin_transform->rect.width / 2.0);
    const float y = origin_transform->rect.y + (origin_transform->rect.height / 2.0);
    bullets->bullets[bullets->current] = (ECSPlayerBullet){
        .dir = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(x, y, 16, 8),
        .draw_conf = {.color = c},
    };
    bullets->current = (bullets->current + 1) % MAX_BULLETS;
}

void bullets_update(Bullets *bullets, float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        ECSPlayerBullet *bullet = &bullets->bullets[i];
        if (time_delta(bullet->creation_time) < BULLET_LIFETIME) {
            if (bullet->dir == BD_RIGHT) {
                bullet->transform.rect.x += 500 * dt;
            } else {
                bullet->transform.rect.x -= 500 * dt;
            }
        }
    }
}
void bullets_draw(const Bullets *bullets) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (time_delta(bullets->bullets[i].creation_time) < BULLET_LIFETIME) {
            draw_solid(&bullets->bullets[i].transform, &bullets->bullets[i].draw_conf);
        }
    }
}
