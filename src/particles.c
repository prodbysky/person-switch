#include "particles.h"
#include "ecs.h"
#include "raylib.h"
#include "raymath.h"
#include "stb_ds_helper.h"
#include "timing_utilities.h"
#include <stb_ds.h>
#include <stddef.h>

void particles_push(Particles *particles, Particle p) {
    stbds_arrput((*particles), p);
}

void particles_spawn_n_in_dir(Particles *particles, int n, Color c, Vector2 dir, Vector2 pos) {
    Particle base_particle = {
        .created_at = GetTime(),
        .active = true,
        .color = c,
        .transform = TRANSFORM(pos.x, pos.y, 8, 8),
        .physics = DEFAULT_PHYSICS(),
    };
    for (int i = 0; i < n; i++) {
        const Vector2 unique_rotation =
            Vector2Scale(Vector2Rotate(dir, GetRandomValue(-100, 100) / 100.0), PARTICLE_VELOCITY);
        const Color unique_color = ColorBrightness(c, GetRandomValue(0, 20) / 100.0);
        particles_push(particles, (Particle){.active = true,
                                             .created_at = base_particle.created_at + GetRandomValue(0, 10) / 10.0,
                                             .physics = (PhysicsComp){.grounded = false, .velocity = unique_rotation},
                                             .color = unique_color,
                                             .transform = base_particle.transform});
    }
}
void particles_draw(const Particles *particles) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*particles); i++) {
        const Particle *p = &(*particles)[i];
        if (p->active) {
            DrawRectangleRec(p->transform.rect,
                             ColorAlpha(p->color, ((time_delta(p->created_at) / PARTICLE_LIFETIME) - 1) * -1));
        }
    }
}
void particles_update(Particles *particles, const Stage *stage, float dt) {
    const ptrdiff_t len = stbds_arrlen(*particles);
    if (len > 300) {
        STB_DS_ARRAY_CLEAN((*particles), !(*particles)[i].active);
    }
    for (ptrdiff_t i = 0; i < stbds_arrlen(*particles); i++) {
        Particle *p = &(*particles)[i];
        if (!p->active) {
            continue;
        }
        physics(&p->physics, dt);
        collision(&p->transform, &p->physics, stage, dt);
        if (time_delta(p->created_at) > PARTICLE_LIFETIME) {
            p->active = false;
        }
    }
}
