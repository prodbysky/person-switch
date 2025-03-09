#ifndef ECS_H
#define ECS_H

#include <raylib.h>
#include "stage.h"

// Base Components

typedef struct {
    Rectangle rect;
} TransformComp;


typedef struct {
    Vector2 velocity;
    bool grounded;
} PhysicsComp;

typedef struct {
    Color color;
} SolidRectangleComp;

#define TRANSFORM(x_, y_, w_, h_) (TransformComp){.rect = {.x = (x_), .y = (y_), .width = (w_), .height = (h_)}}
#define DEFAULT_PHYSICS() (PhysicsComp){.velocity = {.x = 0, .y = 0}, .grounded = false}


// Applies gravity and fades x velocity towards 0
void physics(PhysicsComp* physics, float dt); 

// Essencially physics += vec;
void physics_add_velocity(PhysicsComp* physics, Vector2 vec);

// Essencially physics.x += x;
void physics_add_velocity_x(PhysicsComp* physics, float x);

// Essencially physics.y += y;
void physics_add_velocity_y(PhysicsComp* physics, float y);

// Essencially physics = vec;
void physics_set_velocity(PhysicsComp* physics, Vector2 vec);

// Essencially physics.x = x;
void physics_set_velocity_x(PhysicsComp* physics, float x);

// Essencially physics.y = y;
void physics_set_velocity_y(PhysicsComp* physics, float y);


// Does collision for the passed in transform with the current stage
void collision(TransformComp* transform, PhysicsComp* physics, 
                    const Stage* stage, float dt); 

// Draws a solid rectangle at the specified transform with the color in `SolidRectangle`
void draw_solid(const TransformComp* transform, const SolidRectangleComp* solid_rectangle);

#endif
