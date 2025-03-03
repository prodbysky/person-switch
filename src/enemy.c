#include "enemy.h"
#include "ecs.h"
#include "raylib.h"
#include <math.h>
#include <raymath.h>

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed) {
    return (ECSEnemy){
        .physics = {.grounded = false, .velocity = Vector2Zero()},
        .collider = {.rect = {.x = pos.x, .y = pos.y, .width = size.x, .height = size.y}},
        .transform = {.position = {.x = pos.x, .y = pos.y}},
        .enemy_conf = {.speed = speed},
    };
}
void enemy_ai_system(const EnemyConfigComp *conf, PhysicsComp *physics, const ColliderComp *collider,
                     const ColliderComp *player_collider) {
    float x_pos_delta = fabs(collider->rect.x + (collider->rect.width / 2.0) -
                             (player_collider->rect.x + (player_collider->rect.width / 2.0)));

    if ((player_collider->rect.y + player_collider->rect.height < collider->rect.y) && physics->grounded) {
        physics->velocity.y = -200;
    }
    if (x_pos_delta < 50)
        return;

    if (collider->rect.x + (collider->rect.width / 2.0) >
        player_collider->rect.x + (player_collider->rect.width / 2.0)) {
        physics->velocity.x -= conf->speed;
    }
    if (collider->rect.x + (collider->rect.width / 2.0) <
        player_collider->rect.x + (player_collider->rect.width / 2.0)) {
        physics->velocity.x += conf->speed;
    }
}
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const ColliderComp *player_collider) {
    float dt = GetFrameTime();
    physics_system(&enemy->physics, dt);
    enemy_ai_system(&enemy->enemy_conf, &enemy->physics, &enemy->collider, player_collider);
    collision_system(&enemy->transform, &enemy->physics, &enemy->collider, stage, dt);

    enemy->collider.rect.x = enemy->transform.position.x;
    enemy->collider.rect.y = enemy->transform.position.y;
}
