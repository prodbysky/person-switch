#ifndef ECS_H
#define ECS_H

#include <raylib.h>
#include <stddef.h>
#include "stage.h"

// Base Components

typedef struct {
    Rectangle rect;
} TransformComp;

typedef struct {
    Vector2 velocity;
    bool grounded;
} PhysicsComp;

void physics_system(PhysicsComp* physics, float dt); 

void collision_system(TransformComp* transform, PhysicsComp* physics, 
                    const Stage* stage, float dt); 

#endif
