#include "player.h"
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
    ECSPlayer p = {
        .transform = {.rect = {.width = 32, .height = 96, .x = (WINDOW_W / 2.0f) + 16, .y = (WINDOW_H / 2.0f) + 48}},
        .state = {
            .current_class = PS_MOVE,
            .last_switched = GetTime(),
            .health = 5,
            .last_hit = 0.0,
            .dead = false,
        }};
    return p;
}

void ecs_player_update(ECSPlayer *player, const Stage *stage, const ECSEnemy *enemy) {
    if (player->state.dead) {
        return;
    }
    float dt = GetFrameTime();

    player_input_system(&player->state, &player->physics, &player->transform);
    physics_system(&player->physics, dt);
    collision_system(&player->transform, &player->physics, stage, dt);
    player_enemy_interaction_system(player, enemy);
    if (player->state.health <= 0) {
        player->state.dead = true;
    }
}

void player_enemy_interaction_system(ECSPlayer *player, const ECSEnemy *enemy) {
    if (CheckCollisionRecs(player->transform.rect, enemy->transform.rect) &&
        GetTime() - player->state.last_hit > INVULNERABILITY_TIME) {
        player->state.last_hit = GetTime();
        player->state.health--;
    }
}

void player_input_system(PlayerStateComp *state, PhysicsComp *physics, const TransformComp *transform) {
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
