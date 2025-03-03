#include "ecs.h"
#include "static_config.h"
#include <raymath.h>

void physics_system(PhysicsComp *physics, float dt) {
    physics->velocity.x /= 1.2f;

    if (!physics->grounded) {
        physics->velocity.y += G * dt;
        physics->velocity.y = Clamp(physics->velocity.y, -800, 800);
    } else {
        physics->velocity.y = 0;
    }
}

void collision_system(TransformComp *transform, PhysicsComp *physics, ColliderComp *collider, const Stage *stage,
                      float dt) {
    Vector2 next_pos = {transform->position.x + physics->velocity.x * dt,
                        transform->position.y + physics->velocity.y * dt};

    bool on_any_platform = false;
    Rectangle next_rect = {next_pos.x, next_pos.y, collider->rect.width, collider->rect.height};

    for (int i = 0; i < stage->count; i++) {
        if (CheckCollisionRecs(next_rect, stage->platforms[i])) {
            const Platform *platform = &stage->platforms[i];
            const bool from_left = next_rect.x < platform->x + platform->width;
            const bool from_right = next_rect.x + next_rect.width > platform->x;
            const bool from_bottom = next_rect.y + next_rect.height > platform->y;

            if (from_bottom && physics->velocity.y > 0) {
                physics->velocity.y = 0;
                physics->grounded = true;
                collider->rect.y = platform->y - collider->rect.height;
                on_any_platform = true;
            } else if (from_left && physics->velocity.x < 0) {
                physics->velocity.x = 0;
            } else if (from_right && physics->velocity.x > 0) {
                physics->velocity.x = 0;
            }
        }
    }
    if (physics->grounded && !on_any_platform) {
        bool above_platform = false;
        for (int i = 0; i < stage->count; i++) {
            const Platform *platform = &stage->platforms[i];
            if (collider->rect.x + collider->rect.width > platform->x &&
                collider->rect.x < platform->x + platform->width &&
                collider->rect.y + collider->rect.height <= platform->y &&
                collider->rect.y + collider->rect.height + 1 >= platform->y) {
                above_platform = true;
                break;
            }
        }

        if (!above_platform) {
            physics->grounded = false;
        }
    }

    next_pos = (Vector2){
        .x = collider->rect.x + physics->velocity.x * GetFrameTime(),
        .y = collider->rect.y + physics->velocity.y * GetFrameTime(),
    };

    transform->position = next_pos;
    collider->rect.x = next_pos.x;
    collider->rect.y = next_pos.y;

    collider->rect.x = Clamp(collider->rect.x, 0, WINDOW_W - collider->rect.width);
}
