#include "bullet.h"
#include "ecs.h"
#include "particles.h"
#include "raylib.h"
#include "raymath.h"
#include "stage.h"
#include <math.h>
#include <stb_ds.h>

void bullets_spawn_bullet(Bullets *bullets, Bullet b) {
    stbds_arrput(*bullets, b);
}

void bullets_update(Bullets *bullets, float dt, const Stage *stage, Particles *particles) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*bullets); i++) {
        Bullet *bullet = &(*bullets)[i];
        if (bullet->active) {
            for (size_t j = 0; j < stage->count; j++) {
                // If we collide with ANY of the platforms just die and spawn particles yippie
                if (CheckCollisionRecs(stage->platforms[j], bullet->transform.rect)) {
                    particles_spawn_n_in_dir(particles, 5, bullet->draw_conf.color,
                                             Vector2Rotate(bullet->direction, PI), *(Vector2 *)&bullet->transform);
                    bullet->active = false;
                    break;
                }
            }

            if (!bullet->active) {
                continue;
            }
            const Vector2 movement_delta = Vector2Scale(bullet->direction, dt * bullet->speed);
            const Vector2 next_pos =
                Vector2Add(movement_delta, (Vector2){bullet->transform.rect.x, bullet->transform.rect.y});

            bullet->transform.rect.x = next_pos.x;
            bullet->transform.rect.y = next_pos.y;
        }
    }
}
void bullets_draw(const Bullets *bullets) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*bullets); i++) {
        if ((*bullets)[i].active) {
            Rectangle rect = (*bullets)[i].transform.rect;
            const Vector2 origin = {.x = rect.width / 2.0f, .y = rect.height / 2.0f};

            DrawRectanglePro(rect, origin, atan2((*bullets)[i].direction.y, (*bullets)[i].direction.x) * RAD2DEG,
                             (*bullets)[i].draw_conf.color);
        }
    }
}
