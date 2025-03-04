#include "enemy.h"
#include "ecs.h"
#include "raylib.h"
#include <math.h>
#include <raymath.h>

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed) {
    return (ECSEnemy){
        .physics = {.grounded = false, .velocity = Vector2Zero()},
        .transform = {.rect = {.x = pos.x, .y = pos.y, .width = size.x, .height = size.y}},
        .enemy_conf = {.speed = speed},
    };
}
void enemy_ai_system(const EnemyConfigComp *conf, const TransformComp *transform, PhysicsComp *physics,
                     const TransformComp *player_transform) {
    float x_pos_delta = fabs(transform->rect.x + (transform->rect.width / 2.0) -
                             (player_transform->rect.x + (player_transform->rect.width / 2.0)));

    if ((player_transform->rect.y + player_transform->rect.height < transform->rect.y) && physics->grounded) {
        physics->velocity.y = -200;
    }
    if (x_pos_delta < 50)
        return;

    if (transform->rect.x + (transform->rect.width / 2.0) >
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x -= conf->speed;
    }
    if (transform->rect.x + (transform->rect.width / 2.0) <
        player_transform->rect.x + (player_transform->rect.width / 2.0)) {
        physics->velocity.x += conf->speed;
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform) {
    float dt = GetFrameTime();
    physics_system(&enemy->physics, dt);
    enemy_ai_system(&enemy->enemy_conf, &enemy->transform, &enemy->physics, player_transform);
    collision_system(&enemy->transform, &enemy->physics, stage, dt);
}
