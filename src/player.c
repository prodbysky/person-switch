#include "player.h"
#include "bullet.h"
#include "ecs.h"
#include "enemy.h"
#include "particles.h"
#include "pickup.h"
#include "static_config.h"
#include "timing_utilities.h"
#include <raylib.h>
#include <raymath.h>

ECSPlayer ecs_player_new() {
    ECSPlayer p = {
        .transform = TRANSFORM((GetMonitorWidth(0) / 2.0) + 16, (GetMonitorHeight(0) / 2.0) + 48, 32, 96),
        .state = {.current_class = PS_MOVE,
                  .health = 5,
                  .last_hit = 0.0,
                  .last_shot = 0.0,
                  .last_healed = 0.0,
                  .dead = false,
                  .movement_speed = 0.0,
                  .reload_time = 0.0,
                  .coins = 10},
        .physics = DEFAULT_PHYSICS(),
        .draw_conf = {.color = WHITE},
    };
    return p;
}

void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets,
                       const Sound *jump_sound, const Sound *shoot_sound, Bullets *enemy_bullets, Pickups *pickups,
                       const Camera2D *camera, Particles *particles) {
    if (player->state.dead) {
        return;
    }
    float dt = GetFrameTime();

    if (time_delta(player->state.last_hit) < INVULNERABILITY_TIME) {
        player->draw_conf.color = RED;
    } else if (time_delta(player->state.last_healed) < INVULNERABILITY_TIME) {
        player->draw_conf.color = GetColor(0x55dd55ff);
    } else {
        player->draw_conf.color = WHITE;
    }

    player_input(&player->state, &player->physics, &player->transform, bullets, jump_sound, shoot_sound, camera,
                 particles);
    physics(&player->physics, dt);
    collision(&player->transform, &player->physics, stage, dt);
    player_enemy_interaction(player, wave, enemy_bullets, particles);
    player_pickup_interaction(player, pickups);
    if (player->state.health <= 0) {
        player->state.dead = true;
    }
}

void player_enemy_interaction(ECSPlayer *player, const EnemyWave *wave, Bullets *enemy_bullets, Particles *particles) {
    const float KNOCKBACK_FORCE = 500.0f;

    for (size_t i = 0; i < wave->count; i++) {
        if (wave->enemies[i].state.dead) {
            continue;
        }
        if (CheckCollisionRecs(player->transform.rect, wave->enemies[i].transform.rect) &&
            time_delta(player->state.last_hit) > INVULNERABILITY_TIME) {
            player->state.last_hit = GetTime();
            player->state.health--;

            Vector2 player_center = {player->transform.rect.x + player->transform.rect.width / 2,
                                     player->transform.rect.y + player->transform.rect.height / 2};
            Vector2 enemy_center = {wave->enemies[i].transform.rect.x + wave->enemies[i].transform.rect.width / 2,
                                    wave->enemies[i].transform.rect.y + wave->enemies[i].transform.rect.height / 2};

            Vector2 direction = {0};
            if (player_center.x > enemy_center.x) {
                direction.x = 1.0f;
            } else {
                direction.x = -1.0f;
            }

            const Vector2 scaled_kb = Vector2Scale(direction, KNOCKBACK_FORCE);

            player->physics.velocity.x += scaled_kb.x;

            return;
        }
    }
    for (size_t i = 0; i < MAX_BULLETS; i++) {
        const ECSPlayerBullet *b = &enemy_bullets->bullets[i];
        if (!b->active) {
            continue;
        }
        if (CheckCollisionRecs(player->transform.rect, b->transform.rect) &&
            time_delta(player->state.last_hit) > INVULNERABILITY_TIME) {
            player->state.last_hit = GetTime();
            player->state.health--;
            Vector2 dir = Vector2Rotate(b->direction, PI);
            dir.y *= 2;
            dir.x *= 0.2;
            particles_spawn_n_in_dir(particles, 20, RED, dir, *(Vector2 *)&player->transform);
            return;
        }
    }
}

void player_input(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform, Bullets *bullets,
                  const Sound *jump_sound, const Sound *shoot_sound, const Camera2D *camera, Particles *particles) {

    if (time_delta(state->last_shot) > SHOOT_DELAY - state->reload_time) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
            const Vector2 dir = Vector2Normalize(
                Vector2Subtract(mouse_pos, (Vector2){.x = transform->rect.x + (transform->rect.width / 2.0),
                                                     .y = transform->rect.y + (transform->rect.height / 2.0)}));
            bullets_spawn_bullet(transform, bullets, dir, PURPLE);
            state->last_shot = GetTime();
            PlaySound(*shoot_sound);
        }
    }

    if (IsKeyDown(KEY_A)) {
        physics->velocity.x = -(PLAYER_STATES[state->current_class].speed_x + state->movement_speed);
    }
    if (IsKeyDown(KEY_D)) {
        physics->velocity.x = (PLAYER_STATES[state->current_class].speed_x + state->movement_speed);
    }

    if (physics->grounded && IsKeyPressed(KEY_SPACE)) {
        physics->velocity.y = -PLAYER_STATES[state->current_class].jump_power;
        PlaySound(*jump_sound);
        physics->grounded = false;
        Vector2 pos = *(Vector2 *)transform;
        pos.y += transform->rect.height / 2.0;
        pos.x += transform->rect.width / 2.0;
        particles_spawn_n_in_dir(particles, 5, WHITE, (Vector2){0, 1}, pos);
    }
}

void player_draw(const ECSPlayer *player) {
    if (!player->state.dead) {
        draw_solid(&player->transform, &player->draw_conf);
    }
}
void player_pickup_interaction(ECSPlayer *player, Pickups *pickups) {
    for (size_t i = 0; i < MAX_PICKUPS; i++) {
        Pickup *p = &pickups->pickups[i];
        if (p->active) {
            if (CheckCollisionRecs(p->transform.rect, player->transform.rect)) {
                const double T = GetTime();
                switch (p->type) {
                case PT_HEALTH: {
                    player->state.health += p->health;
                    player->state.last_healed = T;
                    break;
                }
                case PT_COIN: {
                    player->state.coins += p->coin;
                    break;
                }
                }
                p->active = false;
                p->picked_up_at = T;
            }
        }
    }
}
