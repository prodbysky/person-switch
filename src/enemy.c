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
#include <stb_ds.h>
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
                             .ranged.reload_time = reload_time,
                             .ranged.last_shot = 0.0,
                         });
}

ECSEnemy ecs_drone_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double reload_time,
                         double vertical_offset) {
    return ecs_enemy_new(pos, size, speed,
                         (EnemyState){.type = ET_DRONE,
                                      .health = HEALTH(health, health),
                                      .dead = false,
                                      .last_hit = 0.0,
                                      .ranged.reload_time = reload_time,
                                      .ranged.last_shot = 0.0,
                                      .flying = {.vertical_offset = vertical_offset}});
}

ECSEnemy ecs_wolf_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double charge_force, double charge_from,
                        double charge_cooldown) {
    return ecs_enemy_new(pos, size, speed,
                         (EnemyState){.type = ET_WOLF,
                                      .health = HEALTH(health, health),
                                      .dead = false,
                                      .last_hit = 0.0,
                                      .charging = {.charge_from = charge_from,
                                                   .charge_force = charge_force,
                                                   .charge_cooldown = charge_cooldown,
                                                   .last_charged = 0.0}});
}

ECSEnemy ecs_healing_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double heal_amount,
                           double heal_radius) {
    return ecs_enemy_new(pos, size, speed,
                         (EnemyState){.type = ET_HEALER,
                                      .health = HEALTH(health, health),
                                      .dead = false,
                                      .last_hit = 0.0,
                                      .healing = {.heal_amount = heal_amount, .heal_radius = heal_radius}});
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

static void approach_player(const TransformComp *transform, PhysicsComp *physics, const EnemyConfigComp *conf,
                            const TransformComp *player_transform) {
    if (transform->rect.x + (transform->rect.width / 2.0) >
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x -= conf->speed;
    }
    if (transform->rect.x + (transform->rect.width / 2.0) <
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x += conf->speed;
    }
}

static void avoid_player(const TransformComp *transform, PhysicsComp *physics, const EnemyConfigComp *conf,
                         const TransformComp *player_transform, float prefered_range) {
    float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                             (player_transform->rect.x + (player_transform->rect.width / 2.0)));

    const bool player_is_on_the_left = transform->rect.x < player_transform->rect.x;
    if (x_pos_delta > prefered_range + GetRandomValue(-100, 100)) {
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
}

static void shoot_at(EnemyState *state, const TransformComp *transform, const PhysicsComp *player_physics,
                     const TransformComp *player_transform, Bullets *enemy_bullets,
                     Bullet (*create_bullet)(Vector2, Color, Vector2)) {
    if (time_delta(state->ranged.last_shot) > state->ranged.reload_time) {
        const Vector2 player_center = transform_center(player_transform);
        const float dst = Vector2Distance(player_center, transform_center(transform));
        const double time = (dst / RANGER_BULLET_SPEED) - 1;
        const Vector2 prediction = Vector2Add(player_center, Vector2Scale(player_physics->velocity, time));
        const Vector2 dir = Vector2Normalize(Vector2Subtract(prediction, transform_center(transform)));
        bullets_spawn_bullet(enemy_bullets, create_bullet(transform_center(transform), PINK, dir));
        state->ranged.last_shot = GetTime();
    }
}

static void charge(EnemyState *state, const TransformComp *transform, PhysicsComp *physics,
                   const TransformComp *player_transform) {
    const bool player_is_on_the_left = transform->rect.x < player_transform->rect.x;

    float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                             (player_transform->rect.x + (player_transform->rect.width / 2.0)));
    if (x_pos_delta > state->charging.charge_from &&
        time_delta(state->charging.last_charged) > state->charging.charge_cooldown) {
        state->charging.last_charged = GetTime();
        if (player_is_on_the_left) {
            physics->velocity.x = state->charging.charge_force;
        } else {
            physics->velocity.x = -state->charging.charge_force;
        }
        physics->velocity.y -= 500;
    }
}

void enemy_ai(const EnemyConfigComp *conf, EnemyState *state, const TransformComp *transform, PhysicsComp *physics,
              const TransformComp *player_transform, const PhysicsComp *player_physics, Bullets *enemy_bullets,
              ECSEnemy *other_enemies, ptrdiff_t other_enemies_len) {
    switch (state->type) {
    case ET_BASIC: {
        if ((player_transform->rect.y + player_transform->rect.height < transform->rect.y) && physics->grounded) {
            physics->velocity.y = -200;
        }
        approach_player(transform, physics, conf, player_transform);
        break;
    }
    case ET_RANGER: {
        avoid_player(transform, physics, conf, player_transform, 300.0);
        shoot_at(state, transform, player_physics, player_transform, enemy_bullets, ranger_create_bullet);
        break;
    }
    case ET_DRONE: {
        float y_pos_delta = fabs(transform->rect.y + (transform->rect.height / 2.0) -
                                 (player_transform->rect.y + (player_transform->rect.height / 2.0)));
        if (y_pos_delta < state->flying.vertical_offset) {
            physics->velocity.y -= 20;
        }

        approach_player(transform, physics, conf, player_transform);
        shoot_at(state, transform, player_physics, player_transform, enemy_bullets, ranger_create_bullet);
        break;
    }
    case ET_WOLF: {
        approach_player(transform, physics, conf, player_transform);
        charge(state, transform, physics, player_transform);
        break;
    }
    case ET_HEALER: {
        for (ptrdiff_t i = 0; i < other_enemies_len; i++) {
            if (CheckCollisionCircleRec(transform_center(transform), state->healing.heal_radius,
                                        other_enemies[i].transform.rect)) {
                other_enemies[i].state.health.current += state->healing.heal_amount * GetFrameTime();
                other_enemies[i].state.health.current =
                    Clamp(other_enemies[i].state.health.current, 0, other_enemies[i].state.health.max);
            }
        }
        avoid_player(transform, physics, conf, player_transform, 400.0);
    }
    case ET_COUNT: {
    }
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform,
                      const PhysicsComp *player_physics, Bullets *bullets, const Sound *hit_sound,
                      const Sound *death_sound, Bullets *enemy_bullets, Pickups *pickups, Particles *particles,
                      ECSEnemy *other_enemies, ptrdiff_t other_enemies_len) {
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
             enemy_bullets, other_enemies, other_enemies_len);
    collision(&enemy->transform, &enemy->physics, stage, dt);
    enemy_bullet_interaction(&enemy->physics, &enemy->state.health, &enemy->transform, bullets, &enemy->state,
                             hit_sound, particles);
}

void enemy_bullet_interaction(PhysicsComp *physics, HealthComp *health, const TransformComp *transform,
                              Bullets *bullets, EnemyState *state, const Sound *hit_sound, Particles *particles) {
    if (state->health.current <= 0) {
        return;
    }
    for (ptrdiff_t i = 0; i < stbds_arrlen(*bullets); i++) {
        Bullet *bullet = &(*bullets)[i];
        if (bullet->active) {
            if (CheckCollisionRecs(transform->rect, bullet->transform.rect)) {
                bullet->on_hit(bullet, physics, health);
                state->last_hit = GetTime();
                physics->velocity.x += 200 * bullet->direction.x;
                physics->velocity.y += 200 * bullet->direction.y;

                Vector2 pos = *(Vector2 *)transform;
                pos.y += transform->rect.height / 2.0;
                pos.x += transform->rect.width / 2.0;
                particles_spawn_n_in_dir(particles, 5, RED, Vector2Rotate(bullet->direction, PI), pos);
                PlaySound(*hit_sound);
                return;
            }
        }
    }
}

void enemy_draw_self(const ECSEnemy *enemy) {
    draw_solid(&enemy->transform, &enemy->draw_conf);
}
void enemy_draw_health_bar(const ECSEnemy *enemy) {
    DrawRectangleRec(
        (Rectangle){
            .x = enemy->transform.rect.x - enemy->transform.rect.width / 2.0,
            .y = enemy->transform.rect.y - enemy->transform.rect.height / 1.5,
            .width = enemy->transform.rect.width * 2,
            .height = 16,
        },
        GetColor(0x990000ff));
    DrawRectangleRec(
        (Rectangle){
            .x = enemy->transform.rect.x - enemy->transform.rect.width / 2.0,
            .y = enemy->transform.rect.y - enemy->transform.rect.height / 1.5,
            .width = ((enemy->transform.rect.width * 2) / (float)enemy->state.health.max) * enemy->state.health.current,
            .height = 16,
        },
        RED);
}
