#ifndef BULLET_H
#define BULLET_H

#include "ecs.h"

#define MAX_BULLETS 100
#define BULLET_LIFETIME 2.0

typedef enum {
    BD_LEFT,
    BD_RIGHT,
} BulletDirection;

typedef struct {
    TransformComp transform;
    SolidRectangleComp draw_conf;
    BulletDirection dir;
    double creation_time;
} ECSPlayerBullet;

typedef struct {
    ECSPlayerBullet bullets[MAX_BULLETS];
    size_t current;
} Bullets;

// Spawns a bullet according to `dir`
// NOTES:
// Hard limit of `MAX_BULLETS` (if more are spawned then they will be overriden)
void bullets_spawn_bullet(const TransformComp *origin_transform, Bullets *bullets, BulletDirection dir, Color c);

void bullets_update(Bullets *bullets, float dt);
void bullets_draw(const Bullets *bullets);

#endif
