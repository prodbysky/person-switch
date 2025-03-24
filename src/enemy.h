#ifndef ENEMY_H
#define ENEMY_H

#include "ecs.h"
#include "bullet.h"
#include "particles.h"
#include "pickup.h"
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    size_t speed;
} EnemyConfigComp;

typedef enum {
    ET_BASIC,
    ET_RANGER,
    ET_DRONE,
    ET_WOLF,
    ET_HEALER,
    // TODO: TYPES
    // ET_STEALTH
    // ET_STUPID
    // ET_SNIPER
    // ET_KABOOM
    ET_COUNT
} EnemyType;

typedef struct {
    EnemyType type;
    HealthComp health;
    double last_hit;
    bool dead;

    struct {
        double reload_time;
        double last_shot;
    } ranged;
    struct {
        double vertical_offset;
    } flying;
    struct {
        double charge_force;
        double charge_from;
        double charge_cooldown;
        double last_charged;
    } charging;
    struct {
        double heal_amount;
        double heal_radius;
    } healing;
} EnemyState;

typedef struct {
    EnemyState state;
    EnemyConfigComp enemy_conf;
    TransformComp transform;
    PhysicsComp physics;
    SolidRectangleComp draw_conf;
} ECSEnemy;


ECSEnemy ecs_enemy_new(Vector2 pos, Vector2 size, size_t speed, EnemyState state);
ECSEnemy ecs_basic_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health);
ECSEnemy ecs_ranger_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double reload_time);
ECSEnemy ecs_drone_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double reload_time, double vertical_offset);
ECSEnemy ecs_wolf_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double charge_force, double charge_from,
                        double charge_cooldown);
ECSEnemy ecs_healing_enemy(Vector2 pos, Vector2 size, size_t speed, size_t health, double heal_amount, double heal_radius);

// Makes the enemy follow the passed in transform `player_transform`
void enemy_ai(const EnemyConfigComp *conf, EnemyState *state, const TransformComp *transform, PhysicsComp *physics,
              const TransformComp *player_transform, const PhysicsComp *player_physics, Bullets *enemy_bullets, ECSEnemy* other_enemies, ptrdiff_t other_enemies_len);
void ecs_enemy_update(ECSEnemy *enemy, const Stage *stage, const TransformComp *player_transform,
                      const PhysicsComp *player_physics, Bullets *bullets, const Sound *hit_sound,
                      const Sound *death_sound, Bullets *enemy_bullets, Pickups *pickups, Particles *particles, ECSEnemy* other_enemies, ptrdiff_t other_enemies_len);
// Decrements the enemy health after colliding with a single bullet
void enemy_bullet_interaction(PhysicsComp *physics, HealthComp *health, const TransformComp *transform,
                              Bullets *bullets, EnemyState *state, const Sound *hit_sound, Particles *particles);
#endif
