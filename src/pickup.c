#include "pickup.h"
#include "ecs.h"
#include "timing_utilities.h"
#include <raylib.h>
#include <stb_ds.h>

void pickups_spawn(Pickups *pickups, Pickup p) {
    stbds_arrput(*pickups, p);
}

void pickups_draw(const Pickups *pickups) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*pickups); i++) {
        const Pickup *p = &(*pickups)[i];
        if (p->active) {
            DrawRectangleRec(p->transform.rect, WHITE);
        } else if (time_delta(p->picked_up_at) < PICKUP_FADE_OUT_TIME) {
            const double t = time_delta(p->picked_up_at) * (1 / PICKUP_FADE_OUT_TIME);
            DrawRectangleRec(p->transform.rect, GetColor(0xffffffff - (t * 255)));
        }
    }
}
void pickups_update(Pickups *pickups, const Stage *stage, float dt) {
    for (ptrdiff_t i = 0; i < stbds_arrlen(*pickups); i++) {
        Pickup *p = &(*pickups)[i];
        if (p->active) {
            physics(&p->physics, dt);
            collision(&p->transform, &p->physics, stage, dt);
        } else {
            if (time_delta(p->picked_up_at) > PICKUP_FADE_OUT_TIME) {
                stbds_arrdel(*pickups, i);
            }
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

Pickup coin_pickup(float x, float y, float w, float h, size_t coin) {
    return (Pickup){.active = true,
                    .physics = DEFAULT_PHYSICS(),
                    .transform = TRANSFORM(x, y, w, h),
                    .type = PT_COIN,
                    .coin = coin};
}
