#ifndef BULLET_H
#define BULLET_H

#include "particles.h"
#include "ecs.h"

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

typedef Bullet* Bullets;

void bullets_spawn_bullet(Bullets *bullets, Bullet b);

void bullets_update(Bullets *bullets, float dt, const Stage *stage, Particles *particles);
void bullets_draw(const Bullets *bullets);

#endif
