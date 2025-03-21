#ifndef BULLET_H
#define BULLET_H

#include "particles.h"
#include "ecs.h"

#define MAX_BULLETS 100
#define BULLET_LIFETIME 2.0
#define BULLET_SPEED 500

typedef struct {
    TransformComp transform;
    SolidRectangleComp draw_conf;
    Vector2 direction;
    double creation_time;
    bool active;
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
