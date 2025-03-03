#include "player.h"
#include "ecs.h"
#include "raylib.h"
#include "stage.h"
#include "static_config.h"
#include <raymath.h>

static const PlayerStat PLAYER_STATES[] = {
    [PS_TANK] = {.jump_power = 150, .speed_x = 200},
    [PS_MOVE] = {.jump_power = 300, .speed_x = 350},
    [PS_DAMAGE] = {.jump_power = 200, .speed_x = 250},
};

ECSPlayer ecs_player_new() {
    ECSPlayer p = {.collider = {.rect = {.width = 32, .height = 96}},
                   .transform = {.position = {(WINDOW_W / 2.0f) + 16, (WINDOW_H / 2.0f) + 48}},
                   .state = {.current_class = PS_MOVE, .last_switched = GetTime()}};
    p.collider.rect.x = p.transform.position.x;
    p.collider.rect.y = p.transform.position.y;
    return p;
}

void ecs_player_update(ECSPlayer *player, const Stage *stage) {
    float dt = GetFrameTime();

    player_input_system(&player->state, &player->physics, &player->collider);
    physics_system(&player->physics, dt);
    collision_system(&player->transform, &player->physics, &player->collider, stage, dt);

    // Sync Rectangle position
    player->collider.rect.x = player->transform.position.x;
    player->collider.rect.y = player->transform.position.y;
}
void player_input_system(PlayerStateComp *state, PhysicsComp *physics, const ColliderComp *collider) {
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
