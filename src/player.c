#include "player.h"
#include "bullet.h"
#include "ecs.h"
#include "enemy.h"
#include "particles.h"
#include "pickup.h"
#include "static_config.h"
#include "timing_utilities.h"
#include "weapon.h"
#include <raylib.h>
#include <raymath.h>

ECSPlayer ecs_player_new() {
    return (ECSPlayer){.transform =
                           TRANSFORM((GetMonitorWidth(0) / 2.0) + 16, (GetMonitorHeight(0) / 2.0) + 48, 32, 96),
                       .state = {.current_class = PS_MOVE,
                                 .health = 5,
                                 .last_hit = 0.0,
                                 .last_healed = 0.0,
                                 .dead = false,
                                 .movement_speed = 0.0,
                                 .coins = 10},
                       .physics = DEFAULT_PHYSICS(),
                       .draw_conf = {.color = WHITE},

                       .jump_sound = LoadSound("assets/sfx/player_jump.wav"),
                       .selected = 1,
                       .weapons = {create_pistol(), create_ar()}};
}

void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets,
                       Bullets *enemy_bullets, Pickups *pickups, const Camera2D *camera, Particles *particles) {
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

    player_input(player, bullets, camera, particles);
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

            const Vector2 player_center = transform_center(&player->transform);
            const Vector2 enemy_center = transform_center(&wave->enemies[i].transform);

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
        const Bullet *b = &enemy_bullets->bullets[i];
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

void player_input(ECSPlayer *player, Bullets *bullets, const Camera2D *camera, Particles *particles) {
    if (time_delta(player->state.last_shot) > SHOOT_DELAY - player->state.reload_time) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            player->weapons[player->selected].try_shoot(&player->weapons[player->selected], bullets, &player->transform,
                                                        camera);
        }
    }

    if (IsKeyDown(KEY_A)) {
        player->physics.velocity.x =
            -(PLAYER_STATES[player->state.current_class].speed_x + player->state.movement_speed);
    }
    if (IsKeyDown(KEY_D)) {
        player->physics.velocity.x =
            (PLAYER_STATES[player->state.current_class].speed_x + player->state.movement_speed);
    }
    if (IsKeyPressed(KEY_Z)) {
        player->selected = WT_PISTOL;
    }
    if (IsKeyPressed(KEY_X)) {
        player->selected = WT_AR;
    }

    if (player->physics.grounded && IsKeyPressed(KEY_SPACE)) {
        player->physics.velocity.y = -PLAYER_STATES[player->state.current_class].jump_power;
        PlaySound(player->jump_sound);
        player->physics.grounded = false;
        const Vector2 pos = transform_center(&player->transform);
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
