#ifndef ECS_H
#define ECS_H

#include "stage.h"

// Base Components

typedef struct {
    Rectangle rect;
} TransformComp;


typedef struct {
    Vector2 velocity;
    bool grounded;
} PhysicsComp;

#define TRANSFORM(x_, y_, w_, h_) (TransformComp){.rect = {.x = (x_), .y = (y_), .width = (w_), .height = (h_)}}
#define DEFAULT_PHYSICS() (PhysicsComp){.velocity = {.x = 0, .y = 0}, .grounded = false}

void physics_system(PhysicsComp* physics, float dt); 

void collision_system(TransformComp* transform, PhysicsComp* physics, 
                    const Stage* stage, float dt); 

#endif
