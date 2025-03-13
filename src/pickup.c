#include "pickup.h"
#include "ecs.h"
#include "raylib.h"
#include "timing_utilities.h"

void pickups_spawn(Pickups *pickups, Pickup p) {
    pickups->pickups[pickups->current] = p;
    pickups->current = (pickups->current + 1) % MAX_PICKUPS;
}

void pickups_draw(const Pickups *pickups) {
    for (size_t i = 0; i < MAX_PICKUPS; i++) {
        const Pickup *p = &pickups->pickups[i];
        if (p->active) {
            DrawRectangleRec(p->transform.rect, WHITE);
        } else if (time_delta(p->picked_up_at) < PICKUP_FADE_OUT_TIME) {
            const double t = time_delta(p->picked_up_at) * (1 / PICKUP_FADE_OUT_TIME);
            DrawRectangleRec(p->transform.rect, GetColor(0xffffffff - (t * 255)));
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

Pickup health_pickup(float x, float y, float w, float h, size_t health) {
    return (Pickup){.active = true,
                    .physics = DEFAULT_PHYSICS(),
                    .transform = TRANSFORM(x, y, w, h),
                    .type = PT_HEALTH,
                    .health = health};
}
