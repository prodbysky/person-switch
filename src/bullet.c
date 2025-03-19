#include "bullet.h"
#include "ecs.h"
#include "particles.h"
#include "raylib.h"
#include "raymath.h"
#include "stage.h"
#include <math.h>

void bullets_spawn_bullet(const TransformComp *origin_transform, Bullets *bullets, Vector2 dir, Color c) {
    const float x = origin_transform->rect.x + (origin_transform->rect.width / 2.0);
    const float y = origin_transform->rect.y + (origin_transform->rect.height / 2.0);
    bullets->bullets[bullets->current] = (ECSPlayerBullet){
        .direction = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(x, y, 16, 8),
        .draw_conf = {.color = c},
        .active = true,
    };
    bullets->current = (bullets->current + 1) % MAX_BULLETS;
}

void bullets_update(Bullets *bullets, float dt, const Stage *stage, Particles *particles) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        ECSPlayerBullet *bullet = &bullets->bullets[i];
        if (bullet->active) {
            for (size_t j = 0; j < stage->count; j++) {
                if (CheckCollisionRecs(stage->platforms[j], bullet->transform.rect)) {
                    particles_spawn_n_in_dir(particles, 5, bullet->draw_conf.color,
                                             Vector2Rotate(bullet->direction, PI), *(Vector2 *)&bullet->transform);
                    bullet->active = false;
                    break;
                }
            }
            if (!bullet->active) {
                continue;
            }
            const Vector2 movement_delta =
                Vector2Multiply(bullet->direction, (Vector2){dt * BULLET_SPEED, dt * BULLET_SPEED});
            const Vector2 next_pos =
                Vector2Add(movement_delta, (Vector2){bullet->transform.rect.x, bullet->transform.rect.y});
            bullet->transform.rect.x = next_pos.x;
            bullet->transform.rect.y = next_pos.y;
        }
    }
}
void bullets_draw(const Bullets *bullets) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets->bullets[i].active) {
            Rectangle rect = bullets->bullets[i].transform.rect;

            Vector2 origin = {.x = rect.width / 2.0f, .y = rect.height / 2.0f};

            DrawRectanglePro(rect, origin,
                             atan2(bullets->bullets[i].direction.y, bullets->bullets[i].direction.x) * RAD2DEG,
                             bullets->bullets[i].draw_conf.color);
        }
    }
}
