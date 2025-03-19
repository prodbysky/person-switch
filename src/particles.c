#include "particles.h"
#include "ecs.h"
#include "raylib.h"
#include "raymath.h"
#include "timing_utilities.h"

void particles_push(Particles *particles, Particle p) {
    particles->particles[(particles->count++) % MAX_PARTICLES] = p;
}

void particles_spawn_n_in_dir(Particles *particles, int n, Color c, Vector2 dir, Vector2 pos) {
    Particle base_particle = {
        .created_at = GetTime(),
        .active = true,
        .color = c,
        .transform = TRANSFORM(pos.x, pos.y, 16, 16),
        .physics = DEFAULT_PHYSICS(),
    };
    for (int i = 0; i < n; i++) {
        const Vector2 unique_rotation = Vector2Rotate(dir, GetRandomValue(-30 * DEG2RAD, 30 * DEG2RAD));
        const Color unique_color = ColorBrightness(c, GetRandomValue(0, 20) / 10.0);
        particles_push(particles, (Particle){.created_at = base_particle.created_at + GetRandomValue(0, 10) / 10.0,
                                             .physics = (PhysicsComp){.grounded = false,
                                                                      .velocity = Vector2Scale(unique_rotation,
                                                                                               PARTICLE_VELOCITY)},
                                             .color = unique_color,
                                             .transform = base_particle.transform});
    }
}
void particles_draw(const Particles *particles) {
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &particles->particles[i];
        if (p->active) {
            DrawRectangleRec(p->transform.rect, p->color);
        }
    }
}
void particles_update(Particles *particles, const Stage *stage, float dt) {
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles->particles[i];
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
