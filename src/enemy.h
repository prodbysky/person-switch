#ifndef ENEMY_H
#define ENEMY_H

#include "ecs.h"
#include "bullet.h"

typedef struct {
    size_t speed;
} EnemyConfigComp;

typedef struct {
    EnemyConfigComp enemy_conf;
    TransformComp transform;
    PhysicsComp physics;
    size_t health;
    double last_hit;
    bool dead;
} ECSEnemy;

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed);
void enemy_ai_system(const EnemyConfigComp *conf, const TransformComp *transform, PhysicsComp *physics,
                     const TransformComp *player_transform);
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform, Bullets *bullets);
void enemy_bullet_interaction_system(const TransformComp *transform, size_t *health, Bullets *bullets,
                                     double *last_hit);
#endif
