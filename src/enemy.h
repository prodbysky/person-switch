#ifndef ENEMY_H
#define ENEMY_H

#include "ecs.h"
#include "bullet.h"
#include <stdlib.h>

typedef struct {
    size_t speed;
} EnemyConfigComp;

typedef struct {
    int health;
    double last_hit;
    bool dead;
} EnemyState;


typedef enum {
    ET_BASIC,
} EnemyType;

typedef struct {
    EnemyState state;
    EnemyConfigComp enemy_conf;
    TransformComp transform;
    PhysicsComp physics;
    SolidRectangleComp draw_conf;
    EnemyType type;
} ECSEnemy;


ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed, size_t health, EnemyType type);
ECSEnemy ecs_basic_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health);

// Makes the enemy follow the passed in transform `player_transform`
void enemy_ai(const EnemyConfigComp *conf, const TransformComp *transform, PhysicsComp *physics,
                     const TransformComp *player_transform);
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform, Bullets *bullets,
                      const Sound *hit_sound, size_t dmg);
// Decrements the enemy health after colliding with a single bullet
void enemy_bullet_interaction(PhysicsComp *physics, const TransformComp *transform, Bullets *bullets, EnemyState *state,
                              const Sound *hit_sound, size_t dmg);
#endif
