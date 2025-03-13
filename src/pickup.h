#ifndef PICKUP_H
#define PICKUP_H
#include "ecs.h"
#include "stage.h"

typedef enum {
    PT_HEALTH,
} PickupType;

typedef struct {
    PhysicsComp physics;
    TransformComp transform;
    bool active;
    double picked_up_at;

    PickupType type;
    union {
        size_t health;
    };
} Pickup;

#define MAX_PICKUPS 50
#define PICKUP_FADE_OUT_TIME 0.25

typedef struct {
    Pickup pickups[MAX_PICKUPS];
    size_t current;
} Pickups;

Pickup health_pickup(size_t health);

void pickups_spawn(Pickups *pickups, Pickup p); 
void pickups_draw(const Pickups* pickups);
void pickups_update(Pickups *pickups, const Stage *stage, float dt);

#endif
