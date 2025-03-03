#ifndef ECS_H
#define ECS_H

#include <raylib.h>
#include <stddef.h>
#include "stage.h"

// Base Components

typedef struct {
    Vector2 position;
} TransformComp;

typedef struct {
    Rectangle rect;
} ColliderComp;

typedef struct {
    Vector2 velocity;
    bool grounded;
} PhysicsComp;

void physics_system(PhysicsComp* physics, float dt); 

void collision_system(TransformComp* transform, PhysicsComp* physics, ColliderComp* collider, 
                    const Stage* stage, float dt); 

#endif
