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
                             .health = HEALTH(health, health),
                             .dead = false,
                             .last_hit = 0.0,
                         });
}

ECSEnemy ecs_ranger_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double reload_time) {
    return ecs_enemy_new(pos, size, speed,
                         (EnemyState){
                             .type = ET_RANGER,
                             .health = HEALTH(health, health),
                             .dead = false,
                             .last_hit = 0.0,
                             .type_specific.ranger.reload_time = reload_time,
                             .type_specific.ranger.last_shot = 0.0,
                         });
}
void ranger_bullet_on_hit(Bullet *this, PhysicsComp *victim_physics, HealthComp *victim_health) {
    victim_health->current -= this->damage;
    this->active = false;
    victim_physics->velocity.x += 200 * this->direction.x;
    victim_physics->velocity.y += 200 * this->direction.y;
}

#define RANGER_BULLET_SPEED 600
static Bullet ranger_create_bullet(Vector2 pos, Color c, Vector2 dir) {
    return (Bullet){
        .direction = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(pos.x, pos.y, 16, 10),
        .draw_conf = {.color = c},
        .active = true,
        .damage = 1,
        .on_hit = ranger_bullet_on_hit,
        .speed = RANGER_BULLET_SPEED,
    };
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
            const Vector2 player_center = transform_center(player_transform);
            const float dst = Vector2Distance(player_center, transform_center(transform));
            const double time = (dst / RANGER_BULLET_SPEED) - 1;
            const Vector2 prediction = Vector2Add(player_center, Vector2Scale(player_physics->velocity, time));
            const Vector2 dir = Vector2Normalize(Vector2Subtract(prediction, transform_center(transform)));
            bullets_spawn_bullet(enemy_bullets, ranger_create_bullet(transform_center(transform), PINK, dir));
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
                      const Sound *death_sound, Bullets *enemy_bullets, Pickups *pickups, Particles *particles) {
    if (enemy->state.dead) {
        return;
    }
    if (enemy->state.health.current <= 0) {
        pickups_spawn(pickups, coin_pickup(enemy->transform.rect.x, enemy->transform.rect.y, 16, 16, 3));
        if (GetRandomValue(0, 100) < 20) {
            pickups_spawn(pickups, health_pickup(enemy->transform.rect.x, enemy->transform.rect.y, 16, 16, 1));
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
    enemy_bullet_interaction(&enemy->physics, &enemy->state.health, &enemy->transform, bullets, &enemy->state,
                             hit_sound, particles);
}

void enemy_bullet_interaction(PhysicsComp *physics, HealthComp *health, const TransformComp *transform,
                              Bullets *bullets, EnemyState *state, const Sound *hit_sound, Particles *particles) {
    if (state->health.current <= 0) {
        return;
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets->bullets[i].active) {
            if (CheckCollisionRecs(transform->rect, bullets->bullets[i].transform.rect)) {
                bullets->bullets[i].on_hit(&bullets->bullets[i], physics, health);
                state->last_hit = GetTime();
                physics->velocity.x += 200 * bullets->bullets[i].direction.x;
                physics->velocity.y += 200 * bullets->bullets[i].direction.y;

                Vector2 pos = *(Vector2 *)transform;
                pos.y += transform->rect.height / 2.0;
                pos.x += transform->rect.width / 2.0;
                particles_spawn_n_in_dir(particles, 5, RED, Vector2Rotate(bullets->bullets[i].direction, PI), pos);
                PlaySound(*hit_sound);
                return;
            }
        }
    }
}
