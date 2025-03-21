#ifndef BULLET_H
#define BULLET_H

#include "particles.h"
#include "ecs.h"

#define MAX_BULLETS 100
#define BULLET_LIFETIME 2.0

typedef struct Bullet {
    TransformComp transform;
    SolidRectangleComp draw_conf;
    Vector2 direction;
    double speed;
    double creation_time;
    int damage;
    bool active;
    void (*on_hit)(struct Bullet* this, PhysicsComp* victim_physics, HealthComp* victim_health);
} Bullet;

typedef struct {
    Bullet bullets[MAX_BULLETS];
    size_t current;
} Bullets;

// NOTES:
// Hard limit of `MAX_BULLETS` (if more are spawned then they will be overriden)
void bullets_spawn_bullet(Bullets *bullets, Bullet b);

void bullets_update(Bullets *bullets, float dt, const Stage *stage, Particles *particles);
void bullets_draw(const Bullets *bullets);

#endif
