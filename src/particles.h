#ifndef PARTICLES_H
#define PARTICLES_H

#include "ecs.h"
#include "raylib.h"

#define PARTICLE_LIFETIME 2.0
#define MAX_PARTICLES 500
#define PARTICLE_VELOCITY 500

typedef struct {
    TransformComp transform;
    PhysicsComp physics;
    Color color;
    double created_at;
    bool active;
} Particle;

typedef Particle* Particles;

void particles_push(Particles* particles, Particle p);
void particles_spawn_n_in_dir(Particles *particles, int n, Color c, Vector2 dir, Vector2 pos);
void particles_draw(const Particles *particles);
void particles_update(Particles *particles, const Stage *stage, float dt);

#endif
