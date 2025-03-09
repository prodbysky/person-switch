#include "enemy.h"
#include "bullet.h"
#include "ecs.h"
#include "raylib.h"
#include "static_config.h"
#include "timing_utilities.h"
#include <assert.h>
#include <math.h>
#include <raymath.h>
#include <stdlib.h>

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed, EnemyState state) {
    return (ECSEnemy){
        .physics = DEFAULT_PHYSICS(),
        .transform = TRANSFORM(pos.x, pos.y, size.x, size.y),
        .enemy_conf = {.speed = speed},
        .state = state,
        .draw_conf = {.color = BLUE},
    };
}

ECSEnemy ecs_basic_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health) {
    return ecs_enemy_new(pos, size, speed,
                         (EnemyState){
                             .type = ET_BASIC,
                             .health = health,
                             .dead = false,
                             .last_hit = 0.0,
                         });
}

ECSEnemy ecs_ranger_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double reload_time) {
    return ecs_enemy_new(pos, size, speed,
                         (EnemyState){
                             .type = ET_RANGER,
                             .health = health,
                             .dead = false,
                             .last_hit = 0.0,
                             .type_specific.ranger.reload_time = reload_time,
                             .type_specific.ranger.last_shot = 0.0,
                         });
}

void enemy_ai(const EnemyConfigComp *conf, EnemyState *state, const TransformComp *transform, PhysicsComp *physics,
              const TransformComp *player_transform, Bullets *enemy_bullets) {
    switch (state->type) {
    case ET_BASIC: {
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
        break;
    }
    case ET_RANGER: {
        float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                                 (player_transform->rect.x + (player_transform->rect.width / 2.0)));

        const bool player_is_on_the_left = transform->rect.x < player_transform->rect.x;
        if (x_pos_delta > 300.0 + GetRandomValue(-100, 100)) {
            // Move towards the player
            if (player_is_on_the_left) {
                physics->velocity.x += conf->speed;
            } else {
                physics->velocity.x -= conf->speed;
            }
        } else {
            // Move away from the player
            if (player_is_on_the_left) {
                physics->velocity.x -= conf->speed;
            } else {
                physics->velocity.x += conf->speed;
            }
        }

        // Shoot
        if (time_delta(state->type_specific.ranger.last_shot) > state->type_specific.ranger.reload_time) {
            if (player_is_on_the_left) {
                bullets_spawn_bullet(transform, enemy_bullets, BD_RIGHT, GREEN);
            } else {
                bullets_spawn_bullet(transform, enemy_bullets, BD_LEFT, GREEN);
            }
            state->type_specific.ranger.last_shot = GetTime();
        }
    }
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform, Bullets *bullets,
                      const Sound *hit_sound, size_t dmg, Bullets *enemy_bullets) {
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
    enemy_ai(&enemy->enemy_conf, &enemy->state, &enemy->transform, &enemy->physics, player_transform, enemy_bullets);
    collision(&enemy->transform, &enemy->physics, stage, dt);
    enemy_bullet_interaction(&enemy->physics, &enemy->transform, bullets, &enemy->state, hit_sound, dmg);
}

void enemy_bullet_interaction(PhysicsComp *physics, const TransformComp *transform, Bullets *bullets, EnemyState *state,
                              const Sound *hit_sound, size_t dmg) {
    if (state->health <= 0) {
        return;
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (time_delta(bullets->bullets[i].creation_time) < BULLET_LIFETIME) {
            if (CheckCollisionRecs(transform->rect, bullets->bullets[i].transform.rect)) {
                state->health -= dmg;
                bullets->bullets[i].creation_time = 0.0;
                state->last_hit = GetTime();
                if (bullets->bullets[i].dir == BD_LEFT) {
                    physics->velocity.x -= 200;
                } else {
                    physics->velocity.x += 200;
                }
                PlaySound(*hit_sound);
                return;
            }
        }
    }
}
