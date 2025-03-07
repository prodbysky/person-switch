#include "bullet.h"

void bullets_spawn_bullet(const TransformComp *player_transform, Bullets *bullets, BulletDirection dir) {
    const float x = player_transform->rect.x + (player_transform->rect.width / 2.0);
    const float y = player_transform->rect.y + (player_transform->rect.height / 2.0);
    bullets->bullets[bullets->current] = (ECSPlayerBullet){
        .dir = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(x, y, 16, 8),
    };
    bullets->current = (bullets->current + 1) % MAX_BULLETS;
}

void bullets_update(Bullets *bullets, float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (GetTime() - bullets->bullets[i].creation_time < BULLET_LIFETIME) {
            if (bullets->bullets[i].dir == BD_RIGHT) {
                bullets->bullets[i].transform.rect.x += 500 * dt;
            } else {
                bullets->bullets[i].transform.rect.x -= 500 * dt;
            }
        }
    }
}
void bullets_draw(const Bullets *bullets) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (GetTime() - bullets->bullets[i].creation_time < BULLET_LIFETIME) {
            DrawRectangleRec(bullets->bullets[i].transform.rect, PURPLE);
        }
    }
}
