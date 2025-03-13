#ifndef PICKUP_H
#define PICKUP_H
#include "ecs.h"
#include "stage.h"

typedef struct {
    PhysicsComp physics;
    TransformComp transform;
    bool active;
} Pickup;

#define MAX_PICKUPS 50
typedef struct {
    Pickup pickups[MAX_PICKUPS];
    size_t current;
} Pickups;

void pickups_spawn(Pickups *pickups, Pickup p); 
void pickups_draw(const Pickups* pickups);
void pickups_update(Pickups *pickups, const Stage *stage, float dt);

Pickup dummy_pickup();
#endif
