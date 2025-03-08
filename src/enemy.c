#include "enemy.h"
#include "bullet.h"
#include "ecs.h"
#include "raylib.h"
#include "static_config.h"
#include "timing_utilities.h"
#include <math.h>
#include <raymath.h>
#include <stdlib.h>

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed, size_t health) {
    return (ECSEnemy){
        .physics = DEFAULT_PHYSICS(),
        .transform = TRANSFORM(pos.x, pos.y, size.x, size.y),
        .enemy_conf = {.speed = speed},
        .state = {.health = health, .last_hit = 0.0, .dead = false},
        .draw_conf = {.color = BLUE},
    };
}

void enemy_ai(const EnemyConfigComp *conf, const TransformComp *transform, PhysicsComp *physics,
              const TransformComp *player_transform) {
    float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                             (player_transform->rect.x + (player_transform->rect.width / 2.0)));

    // Jump if we can and the player bottom y position is higher than our y top position
    if ((player_transform->rect.y + player_transform->rect.height < transform->rect.y) && physics->grounded) {
        physics->velocity.y = -200;
    }
    // If close enough to the player then just return
    if (x_pos_delta < 50) {
        return;
    }

    // Approach player
    if (transform->rect.x + (transform->rect.width / 2.0) >
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x -= conf->speed;
    }
    if (transform->rect.x + (transform->rect.width / 2.0) <
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x += conf->speed;
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform, Bullets *bullets,
                      const Sound *hit_sound, size_t dmg) {
    if (enemy->state.dead) {
        return;
    }
    if (enemy->state.health <= 0) {
        enemy->state.dead = true;
        return;
    }
    const float dt = GetFrameTime();

    if (time_delta(enemy->state.last_hit) < INVULNERABILITY_TIME) {
        enemy->draw_conf.color = RED;
    } else {
        enemy->draw_conf.color = BLUE;
    }
    physics(&enemy->physics, dt);
    enemy_ai(&enemy->enemy_conf, &enemy->transform, &enemy->physics, player_transform);
    collision(&enemy->transform, &enemy->physics, stage, dt);
    enemy_bullet_interaction(&enemy->transform, bullets, &enemy->state, hit_sound, dmg);
}

void enemy_bullet_interaction(const TransformComp *transform, Bullets *bullets, EnemyState *state,
                              const Sound *hit_sound, size_t dmg) {
    if (state->health <= 0) {
        return;
    }
    if (time_delta(state->last_hit) < INVULNERABILITY_TIME) {
        return;
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (time_delta(bullets->bullets[i].creation_time) < BULLET_LIFETIME) {
            if (CheckCollisionRecs(transform->rect, bullets->bullets[i].transform.rect)) {
                state->health -= dmg;
                bullets->bullets[i].creation_time = INFINITY;
                state->last_hit = GetTime();
                PlaySound(*hit_sound);
                return;
            }
        }
    }
}
