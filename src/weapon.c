#include "weapon.h"
#include "ecs.h"
#include "timing_utilities.h"
#include <raylib.h>
#include <raymath.h>

void pistol_try_shoot(struct Weapon *this, Bullets *bullets, const TransformComp *from, const Camera2D *camera) {
    if (time_delta(this->last_shot) > this->fire_rate) {
        const Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
        const Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_pos, transform_center(from)));
        bullets_spawn_bullet(bullets, this->pistol.create_bullet(transform_center(from), PURPLE, dir));
        this->last_shot = GetTime();
        PlaySound(this->pistol.shoot_sound);
    }
}

void pistol_on_hit(Bullet *this, PhysicsComp *victim_physics, HealthComp *victim_health) {
    victim_health->current -= this->damage;
    this->active = false;
    victim_physics->velocity.x += 200 * this->direction.x;
    victim_physics->velocity.y += 200 * this->direction.y;
}

Bullet pistol_create_bullet(Vector2 pos, Color c, Vector2 dir) {
    return (Bullet){
        .direction = dir,
        .creation_time = GetTime(),
        .transform = TRANSFORM(pos.x, pos.y, 16, 8),
        .draw_conf = {.color = c},
        .damage = 3,
        .active = true,
        .on_hit = pistol_on_hit,
    };
}

Weapon create_pistol(double file_rate) {
    return (Weapon){
        .type = WT_PISTOL,
        .last_shot = 0.0,
        .fire_rate = file_rate,
        .damage = 3,
        .pistol =
            {
                .shoot_sound = LoadSound("assets/sfx/pistol_shoot.wav"),
                .try_shoot = pistol_try_shoot,
                .create_bullet = pistol_create_bullet,
            },
    };
}
