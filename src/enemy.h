#ifndef ENEMY_H
#define ENEMY_H


#include "ecs.h"
#include <raylib.h>
typedef struct {
    size_t speed;
} EnemyConfigComp;

typedef struct {
    EnemyConfigComp enemy_conf;
    TransformComp transform;
    PhysicsComp physics;
} ECSEnemy;

ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed);
void enemy_ai_system(const EnemyConfigComp *conf, const TransformComp *transform, PhysicsComp *physics,
                     const TransformComp *player_transform);
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform);
#endif
