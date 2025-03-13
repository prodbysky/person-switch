#include "pickup.h"
#include "ecs.h"
#include "raylib.h"

void pickups_spawn(Pickups *pickups, Pickup p) {
    pickups->pickups[pickups->current] = p;
    pickups->current = (pickups->current + 1) % MAX_PICKUPS;
}

void pickups_draw(const Pickups *pickups) {
    for (size_t i = 0; i < MAX_PICKUPS; i++) {
        const Pickup *p = &pickups->pickups[i];
        if (p->active) {
            DrawRectangleRec(p->transform.rect, WHITE);
        }
    }
}
void pickups_update(Pickups *pickups, const Stage *stage, float dt) {
    for (size_t i = 0; i < MAX_PICKUPS; i++) {
        Pickup *p = &pickups->pickups[i];
        if (p->active) {
            physics(&p->physics, dt);
            collision(&p->transform, &p->physics, stage, dt);
        }
    }
}

Pickup dummy_pickup() {
    return (Pickup){.active = true, .physics = DEFAULT_PHYSICS(), .transform = TRANSFORM(200, 200, 64, 64)};
}
