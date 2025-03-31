#ifndef WEAPON_H
#define WEAPON_H

#include "bullet.h"
#include <stdint.h>
#include "ecs.h"
typedef enum {
    WT_PISTOL,
    WT_AR,
    WT_SHOTGUN,
    WT_COUNT,
} WeaponType;

typedef struct Weapon {
    WeaponType type;
    double last_shot;
    double fire_rate;
    uint16_t damage;
    Sound shoot_sound;
    float fire_rate_upgrade_cost;
    float damage_upgrade_cost;
    void (*try_shoot)(struct Weapon* this, Bullets* bullets, const TransformComp* from, const Camera2D* camera);
    Bullet (*create_bullet)(struct Weapon* this, Vector2 pos, Color c, Vector2 dir);
} Weapon;

Weapon create_pistol();
void pistol_try_shoot(struct Weapon* this, Bullets* bullets, const TransformComp* from, const Camera2D* camera);
Bullet pistol_create_bullet(Weapon *pistol, Vector2 pos, Color c, Vector2 dir);

Weapon create_ar();
void ar_try_shoot(struct Weapon* this, Bullets* bullets, const TransformComp* from, const Camera2D* camera);
Bullet ar_create_bullet(Weapon* ar, Vector2 pos, Color c, Vector2 dir);

Weapon create_shotgun();
void shotgun_try_shoot(struct Weapon* this, Bullets* bullets, const TransformComp* from, const Camera2D* camera);
Bullet shotgun_create_bullet(Weapon* ar, Vector2 pos, Color c, Vector2 dir);

#endif
