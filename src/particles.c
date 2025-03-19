#include "particles.h"
#include "ecs.h"
#include "raylib.h"
#include "timing_utilities.h"

void particles_push(Particles *particles, Particle p) {
    particles->particles[(particles->count++) % MAX_PARTICLES] = p;
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
