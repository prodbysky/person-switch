#include "weapon.h"
#include "timing_utilities.h"
#include <raylib.h>
#include <raymath.h>

void pistol_try_shoot(struct Weapon *this, Bullets *bullets, const TransformComp *from, const Camera2D *camera) {
    if (time_delta(this->pistol.last_shot) > this->pistol.fire_rate) {
        const Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
        const Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_pos, transform_center(from)));
        bullets_spawn_bullet(from, bullets, dir, PURPLE);
        this->pistol.last_shot = GetTime();
        PlaySound(this->pistol.shoot_sound);
    }
}

Weapon create_pistol(double file_rate) {
    return (Weapon){
        .type = WT_PISTOL,
        .pistol =
            {
                .last_shot = 0.0,
                .fire_rate = file_rate,
                .damage = 3,
                .shoot_sound = LoadSound("assets/sfx/pistol_shoot.wav"),
                .try_shoot = pistol_try_shoot,
            },
    };
}
