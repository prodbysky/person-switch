#ifndef BULLET_H
#define BULLET_H

#include "ecs.h"

#define MAX_BULLETS 100

typedef enum {
    BD_LEFT,
    BD_RIGHT,
} BulletDirection;

typedef struct {
    TransformComp transform;
    BulletDirection dir;
    double creation_time;
} ECSPlayerBullet;

typedef struct {
    ECSPlayerBullet bullets[MAX_BULLETS];
    size_t current;
} Bullets;

void bullets_spawn_bullet_system(const TransformComp* player_transform, Bullets *bullets, BulletDirection dir);
void bullets_update_system(Bullets *bullets, float dt);
void bullets_draw_system(const Bullets *bullets);

#endif
