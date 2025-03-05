#include "player.h"
#include "ecs.h"
#include "enemy.h"
#include "raylib.h"
#include "static_config.h"

static const PlayerStat PLAYER_STATES[] = {
    [PS_TANK] = {.jump_power = 150, .speed_x = 200},
    [PS_MOVE] = {.jump_power = 300, .speed_x = 350},
    [PS_DAMAGE] = {.jump_power = 200, .speed_x = 250},
};

#define INVULNERABILITY_TIME 1.0

ECSPlayer ecs_player_new() {
    ECSPlayer p = {.transform = TRANSFORM((WINDOW_W / 2.0) + 16, (WINDOW_H / 2.0) + 48, 32, 96),
                   .state = {
                       .current_class = PS_MOVE,
                       .last_switched = 0.0,
                       .health = 5,
                       .last_hit = 0.0,
                       .last_shot = 0.0,
                       .dead = false,
                   }};
    return p;
}

void ecs_player_update(ECSPlayer *player, const Stage *stage, const EnemyWave *wave, Bullets *bullets) {
    if (player->state.dead) {
        return;
    }
    float dt = GetFrameTime();

    player_input_system(&player->state, &player->physics, &player->transform, bullets);
    physics_system(&player->physics, dt);
    collision_system(&player->transform, &player->physics, stage, dt);
    player_enemy_interaction_system(player, wave);
    if (player->state.health <= 0) {
        player->state.dead = true;
    }
}

void player_enemy_interaction_system(ECSPlayer *player, const EnemyWave *wave) {
    for (int i = 0; i < wave->count; i++) {
        if (CheckCollisionRecs(player->transform.rect, wave->enemies[i].transform.rect) &&
            GetTime() - player->state.last_hit > INVULNERABILITY_TIME) {
            player->state.last_hit = GetTime();
            player->state.health--;
            return;
        }
    }
}

void player_input_system(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform,
                         Bullets *bullets) {

    if (GetTime() - state->last_shot > 0.25) {
        if (IsKeyPressed(KEY_LEFT)) {
            bullets_spawn_bullet_system(transform, bullets, BD_LEFT);
            state->last_shot = GetTime();
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            bullets_spawn_bullet_system(transform, bullets, BD_RIGHT);
            state->last_shot = GetTime();
        }
    }

    if (IsKeyPressed(KEY_F) && GetTime() - state->last_switched > PLAYER_CLASS_SWITCH_COOLDOWN) {
        state->current_class = (state->current_class + 1) % PS_COUNT;
        state->last_switched = GetTime();
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
        physics->grounded = false;
    }
}

void bullets_spawn_bullet_system(const TransformComp *player_transform, Bullets *bullets, BulletDirection dir) {
    const float x = player_transform->rect.x + (player_transform->rect.width / 2.0);
    const float y = player_transform->rect.y + (player_transform->rect.height / 2.0);
    bullets->bullets[bullets->current] = (ECSPlayerBullet){
        .dir = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(x, y, 32, 32),
    };
    bullets->current = (bullets->current + 1) % 100;
}

void bullets_update_system(Bullets *bullets, float dt) {
    for (int i = 0; i < 100; i++) {
        if (GetTime() - bullets->bullets[i].creation_time < 2) {
            if (bullets->bullets[i].dir == BD_RIGHT) {
                bullets->bullets[i].transform.rect.x += 500 * dt;
            } else {
                bullets->bullets[i].transform.rect.x -= 500 * dt;
            }
        }
    }
}
void bullets_draw_system(const Bullets *bullets) {
    for (int i = 0; i < 100; i++) {
        if (GetTime() - bullets->bullets[i].creation_time < 2) {
            DrawRectangleRec(bullets->bullets[i].transform.rect, PURPLE);
        }
    }
}
