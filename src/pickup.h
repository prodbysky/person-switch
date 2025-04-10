#ifndef PICKUP_H
#define PICKUP_H
#include "ecs.h"
#include "stage.h"

typedef enum {
    PT_HEALTH,
    PT_COIN,
} PickupType;

typedef struct {
    PhysicsComp physics;
    TransformComp transform;
    bool active;
    double picked_up_at;

    PickupType type;
    union {
        size_t health;
        size_t coin;
    };
} Pickup;

#define PICKUP_FADE_OUT_TIME 0.25

typedef Pickup* Pickups;

Pickup health_pickup(float x, float y, float w, float h, size_t health);
Pickup coin_pickup(float x, float y, float w, float h, size_t coin);

void pickups_spawn(Pickups *pickups, Pickup p); 
void pickups_draw(const Pickups* pickups);
void pickups_update(Pickups *pickups, const Stage *stage, float dt);

#endif
