#ifndef WEAPON_H
#define WEAPON_H

#include "bullet.h"
#include <stdint.h>
#include "ecs.h"
typedef enum {
    WT_PISTOL
} WeaponType;

typedef struct Weapon{
    WeaponType type;
    union {
        struct {
            double last_shot;
            double fire_rate;
            uint16_t damage;
            Sound shoot_sound;
            void (*try_shoot)(struct Weapon* this, Bullets* bullets, const TransformComp* from, const Camera2D* camera);
            Bullet (*create_bullet)(Vector2 pos, Color c, Vector2 dir);
        } pistol;
    };
} Weapon;

void pistol_try_shoot(struct Weapon* this, Bullets* bullets, const TransformComp* from, const Camera2D* camera);
Bullet pistol_create_bullet(Vector2 pos, Color c, Vector2 dir);

Weapon create_pistol(double file_rate);

#endif
