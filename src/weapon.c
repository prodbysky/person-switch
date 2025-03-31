#include "weapon.h"
#include "ecs.h"
#include "timing_utilities.h"
#include <raylib.h>
#include <raymath.h>

void pistol_try_shoot(struct Weapon *this, Bullets *bullets, const TransformComp *from, const Camera2D *camera) {
    if (time_delta(this->last_shot) > this->fire_rate) {
        const Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
        const Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_pos, transform_center(from)));
        bullets_spawn_bullet(bullets, this->create_bullet(this, transform_center(from), PURPLE, dir));
        this->last_shot = GetTime();
        PlaySound(this->shoot_sound);
    }
}

void pistol_on_hit(Bullet *this, PhysicsComp *victim_physics, HealthComp *victim_health) {
    victim_health->current -= this->damage;
    this->active = false;
    victim_physics->velocity.x += 200 * this->direction.x;
    victim_physics->velocity.y += 200 * this->direction.y;
}

Bullet pistol_create_bullet(Weapon *pistol, Vector2 pos, Color c, Vector2 dir) {
    return (Bullet){
        .direction = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(pos.x, pos.y, 16, 8),
        .draw_conf = {.color = c},
        .damage = pistol->damage,
        .active = true,
        .on_hit = pistol_on_hit,
        .speed = 900,
    };
}

Weapon create_pistol() {
    return (Weapon){
        .type = WT_PISTOL,
        .last_shot = 0.0,
        .fire_rate = 0.5,
        .damage = 4,
        .shoot_sound = LoadSound("assets/sfx/pistol_shoot.wav"),
        .try_shoot = pistol_try_shoot,
        .create_bullet = pistol_create_bullet,
        .damage_upgrade_cost = 2,
        .fire_rate_upgrade_cost = 4,
    };
}
Weapon create_ar() {
    return (Weapon){
        .type = WT_AR,
        .last_shot = 0.0,
        .fire_rate = .25,
        .damage = 2,
        .shoot_sound = LoadSound("assets/sfx/ar_shoot.wav"),
        .try_shoot = ar_try_shoot,
        .create_bullet = ar_create_bullet,
        .damage_upgrade_cost = 3,
        .fire_rate_upgrade_cost = 5,
    };
}

Weapon create_shotgun() {
    return (Weapon){
        .type = WT_SHOTGUN,
        .last_shot = 0.0,
        .fire_rate = 1,
        .damage = 5,
        .shoot_sound = LoadSound("assets/sfx/ar_shoot.wav"),
        .try_shoot = shotgun_try_shoot,
        .create_bullet = shotgun_create_bullet,
        .damage_upgrade_cost = 4,
        .fire_rate_upgrade_cost = 5,
    };
}

void ar_try_shoot(struct Weapon *this, Bullets *bullets, const TransformComp *from, const Camera2D *camera) {
    if (time_delta(this->last_shot) > this->fire_rate) {
        const Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
        const Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_pos, transform_center(from)));
        bullets_spawn_bullet(bullets, this->create_bullet(this, transform_center(from), PURPLE, dir));
        this->last_shot = GetTime();
        PlaySound(this->shoot_sound);
    }
}

void ar_on_hit(Bullet *this, PhysicsComp *victim_physics, HealthComp *victim_health) {
    victim_health->current -= this->damage;
    this->active = false;
    victim_physics->velocity.x += 100 * this->direction.x;
    victim_physics->velocity.y += 100 * this->direction.y;
}

Bullet ar_create_bullet(Weapon *ar, Vector2 pos, Color c, Vector2 dir) {
    return (Bullet){
        .direction = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(pos.x, pos.y, 8, 4),
        .draw_conf = {.color = c},
        .damage = ar->damage,
        .active = true,
        .on_hit = ar_on_hit,
        .speed = 1500,
    };
}

void shotgun_try_shoot(struct Weapon *this, Bullets *bullets, const TransformComp *from, const Camera2D *camera) {
    if (time_delta(this->last_shot) > this->fire_rate) {
        const Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
        for (size_t i = 0; i < 5; i++) {
            const Vector2 dir = Vector2Rotate(Vector2Normalize(Vector2Subtract(mouse_pos, transform_center(from))),
                                              GetRandomValue(-15, 15) * DEG2RAD);
            bullets_spawn_bullet(bullets, this->create_bullet(this, transform_center(from), PURPLE, dir));
        }
        this->last_shot = GetTime();
        PlaySound(this->shoot_sound);
    }
}

void shotgun_on_hit(Bullet *this, PhysicsComp *victim_physics, HealthComp *victim_health) {
    victim_health->current -= this->damage;
    this->active = false;
    victim_physics->velocity.x += 100 * this->direction.x;
    victim_physics->velocity.y += 100 * this->direction.y;
}

Bullet shotgun_create_bullet(Weapon *shotgun, Vector2 pos, Color c, Vector2 dir) {
    return (Bullet){
        .direction = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(pos.x, pos.y, 8, 8),
        .draw_conf = {.color = c},
        .damage = shotgun->damage,
        .active = true,
        .on_hit = shotgun_on_hit,
        .speed = 1500,
    };
}
