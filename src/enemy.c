#include "enemy.h"
#include "bullet.h"
#include "ecs.h"
#include "raylib.h"
#include "static_config.h"
#include <math.h>
#include <raymath.h>
#include <stdlib.h>

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed) {
    return (ECSEnemy){.physics = DEFAULT_PHYSICS(),
                      .transform = TRANSFORM(pos.x, pos.y, size.x, size.y),
                      .enemy_conf = {.speed = speed},
                      .health = 5,
                      .last_hit = 0.0,
                      .dead = false,
                      .c = BLUE};
}

void enemy_ai(const EnemyConfigComp *conf, const TransformComp *transform, PhysicsComp *physics,
              const TransformComp *player_transform) {
    float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                             (player_transform->rect.x + (player_transform->rect.width / 2.0)));

    if ((player_transform->rect.y + player_transform->rect.height < transform->rect.y) && physics->grounded) {
        physics->velocity.y = -200;
    }
    if (x_pos_delta < 50)
        return;

    if (transform->rect.x + (transform->rect.width / 2.0) >
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x -= conf->speed;
    }
    if (transform->rect.x + (transform->rect.width / 2.0) <
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x += conf->speed;
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform, Bullets *bullets) {
    float dt = GetFrameTime();
    if (enemy->dead) {
        return;
    }
    if (enemy->health <= 0) {
        enemy->dead = true;
        return;
    }

    if (GetTime() - enemy->last_hit < INVULNERABILITY_TIME) {
        enemy->c = RED;
    } else {
        enemy->c = BLUE;
    }
    physics(&enemy->physics, dt);
    enemy_ai(&enemy->enemy_conf, &enemy->transform, &enemy->physics, player_transform);
    collision(&enemy->transform, &enemy->physics, stage, dt);
    enemy_bullet_interaction(&enemy->transform, &enemy->health, bullets, &enemy->last_hit);
}

void enemy_bullet_interaction(const TransformComp *transform, size_t *health, Bullets *bullets, double *last_hit) {
    if ((*health) <= 0) {
        return;
    }
    if (GetTime() - (*last_hit) < INVULNERABILITY_TIME) {
        return;
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (GetTime() - bullets->bullets[i].creation_time < 2) {
            if (CheckCollisionRecs(transform->rect, bullets->bullets[i].transform.rect)) {
                (*health)--;
                bullets->bullets[i].creation_time = INFINITY;
                *last_hit = GetTime();
                return;
            }
        }
    }
}
