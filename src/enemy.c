#include "enemy.h"
#include "bullet.h"
#include "ecs.h"
#include "particles.h"
#include "pickup.h"
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
              const TransformComp *player_transform, const PhysicsComp *player_physics, Bullets *enemy_bullets) {
    switch (state->type) {
    case ET_BASIC: {
        float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                                 (player_transform->rect.x + (player_transform->rect.width / 2.0)));

        if ((player_transform->rect.y + player_transform->rect.height < transform->rect.y) && physics->grounded) {
            physics->velocity.y = -200;
        }
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
            const Vector2 player_center =
                (Vector2){.x = player_transform->rect.x + (player_transform->rect.width / 2.0),
                          .y = player_transform->rect.y + (player_transform->rect.height / 2.0)};
            const float dst =
                Vector2Distance(player_center, (Vector2){.x = transform->rect.x + (transform->rect.width / 2.0),
                                                         .y = transform->rect.y + (transform->rect.height / 2.0)});
            const double time = (dst / BULLET_SPEED) - 1;
            const Vector2 prediction = Vector2Add(player_center, Vector2Scale(player_physics->velocity, time));
            const Vector2 dir = Vector2Normalize(
                Vector2Subtract(prediction, (Vector2){.x = transform->rect.x + (transform->rect.width / 2.0),
                                                      .y = transform->rect.y + (transform->rect.height / 2.0)}));
            bullets_spawn_bullet(transform, enemy_bullets, dir, GREEN);
            state->type_specific.ranger.last_shot = GetTime();
        }
        break;
    }
    case ET_COUNT: {
    }
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform,
                      const PhysicsComp *player_physics, Bullets *bullets, const Sound *hit_sound,
                      const Sound *death_sound, size_t dmg, Bullets *enemy_bullets, Pickups *pickups,
                      Particles *particles) {
    if (enemy->state.dead) {
        return;
    }
    if (enemy->state.health <= 0) {
        pickups_spawn(pickups, coin_pickup(enemy->transform.rect.x, enemy->transform.rect.y, 16, 16, 3));
        if (GetRandomValue(0, 100) < 20) {
            pickups_spawn(pickups, health_pickup(enemy->transform.rect.x, enemy->transform.rect.y, 16, 16, 3));
        }
        particles_spawn_n_in_dir(particles, 10, RED, (Vector2){0, -0.5}, *(Vector2 *)&enemy->transform);
        enemy->state.dead = true;
        PlaySound(*death_sound);
        return;
    }
    const float dt = GetFrameTime();

    if (time_delta(enemy->state.last_hit) < INVULNERABILITY_TIME) {
        enemy->draw_conf.color = RED;
    } else {
        enemy->draw_conf.color = BLUE;
    }
    physics(&enemy->physics, dt);
    enemy_ai(&enemy->enemy_conf, &enemy->state, &enemy->transform, &enemy->physics, player_transform, player_physics,
             enemy_bullets);
    collision(&enemy->transform, &enemy->physics, stage, dt);
    enemy_bullet_interaction(&enemy->physics, &enemy->transform, bullets, &enemy->state, hit_sound, dmg);
}

void enemy_bullet_interaction(PhysicsComp *physics, const TransformComp *transform, Bullets *bullets, EnemyState *state,
                              const Sound *hit_sound, size_t dmg) {
    if (state->health <= 0) {
        return;
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets->bullets[i].active) {
            if (CheckCollisionRecs(transform->rect, bullets->bullets[i].transform.rect)) {
                state->health -= dmg;
                bullets->bullets[i].active = false;
                state->last_hit = GetTime();
                physics->velocity.x += 200 * bullets->bullets[i].direction.x;
                physics->velocity.y += 200 * bullets->bullets[i].direction.y;
                PlaySound(*hit_sound);
                return;
            }
        }
    }
}
