#include "player.h"
#include "ecs.h"
#include "enemy.h"
#include "static_config.h"
#include <raylib.h>

ECSPlayer ecs_player_new() {
    ECSPlayer p = {
        .transform = TRANSFORM((WINDOW_W / 2.0) + 16, (WINDOW_H / 2.0) + 48, 32, 96),
        .state =
            {
                .current_class = PS_MOVE,
                .health = 5,
                .last_hit = 0.0,
                .last_shot = 0.0,
                .dead = false,
            },
        .physics = DEFAULT_PHYSICS(),
        .c = WHITE,
    };
    return p;
}

void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets,
                       const Sound *jump_sound, const Sound *shoot_sound) {
    if (player->state.dead) {
        return;
    }
    float dt = GetFrameTime();

    if (GetTime() - player->state.last_hit < INVULNERABILITY_TIME) {
        player->c = RED;
    } else {
        player->c = WHITE;
    }

    player_input(&player->state, &player->physics, &player->transform, bullets, jump_sound, shoot_sound);
    physics(&player->physics, dt);
    collision(&player->transform, &player->physics, stage, dt);
    player_enemy_interaction(player, wave);
    if (player->state.health <= 0) {
        player->state.dead = true;
    }
}

void player_enemy_interaction(ECSPlayer *player, const EnemyWave *wave) {
    for (int i = 0; i < wave->count; i++) {
        if (wave->enemies[i].state.dead) {
            continue;
        }
        if (CheckCollisionRecs(player->transform.rect, wave->enemies[i].transform.rect) &&
            GetTime() - player->state.last_hit > INVULNERABILITY_TIME) {
            player->state.last_hit = GetTime();
            player->state.health--;
            return;
        }
    }
}

void player_input(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform, Bullets *bullets,
                  const Sound *jump_sound, const Sound *shoot_sound) {

    if (GetTime() - state->last_shot > 0.25) {
        if (IsKeyPressed(KEY_LEFT)) {
            bullets_spawn_bullet(transform, bullets, BD_LEFT);
            state->last_shot = GetTime();
            PlaySound(*shoot_sound);
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            bullets_spawn_bullet(transform, bullets, BD_RIGHT);
            state->last_shot = GetTime();
            PlaySound(*shoot_sound);
        }
    }

    physics->velocity.x = 0;
    if (IsKeyDown(KEY_A)) {
        physics->velocity.x = -PLAYER_STATES[state->current_class].speed_x;
    }
    if (IsKeyDown(KEY_D)) {
        physics->velocity.x = PLAYER_STATES[state->current_class].speed_x;
    }

    if (physics->grounded && IsKeyPressed(KEY_SPACE)) {
        physics->velocity.y = -PLAYER_STATES[state->current_class].jump_power;
        PlaySound(*jump_sound);
        physics->grounded = false;
    }
}
