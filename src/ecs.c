#include "ecs.h"
#include "raylib.h"
#include "stage.h"
#include "static_config.h"
#include <raymath.h>

Vector2 transform_center(const TransformComp *transform) {
    return (Vector2){
        .x = transform->rect.x + transform->rect.width / 2.0,
        .y = transform->rect.y + transform->rect.height / 2.0,
    };
}

void physics(PhysicsComp *physics, float dt) {
    physics->velocity.x /= 1 + (10 * dt);

    if (!physics->grounded) {
        physics->velocity.y += G * dt;
    }
    /*physics->velocity.x = Clamp(physics->velocity.x, -300, 300);*/
    physics->velocity.y = Clamp(physics->velocity.y, -800, 800);
}

void physics_add_velocity(PhysicsComp *physics, Vector2 vec) {
    physics->velocity = Vector2Add(physics->velocity, vec);
}

void physics_add_velocity_x(PhysicsComp *physics, float x) {
    physics->velocity.x += x;
}

void physics_add_velocity_y(PhysicsComp *physics, float y) {
    physics->velocity.y += y;
}

void physics_set_velocity(PhysicsComp *physics, Vector2 vec) {
    physics->velocity = vec;
}

void physics_set_velocity_x(PhysicsComp *physics, float x) {
    physics->velocity.x = x;
}

void physics_set_velocity_y(PhysicsComp *physics, float y) {
    physics->velocity.y = y;
}

void collision(TransformComp *transform, PhysicsComp *physics, const Stage *stage, float dt) {
    float old_x = transform->rect.x;
    float old_y = transform->rect.y;

    float new_x = transform->rect.x + physics->velocity.x * dt;
    float new_y = transform->rect.y + physics->velocity.y * dt;

    transform->rect.x = new_x;
    for (size_t i = 0; i < stage->count; i++) {
        if (CheckCollisionRecs(transform->rect, stage->platforms[i])) {
            const Platform *platform = &stage->platforms[i];
            if (old_x + transform->rect.width <= platform->x) {
                transform->rect.x = platform->x - transform->rect.width;
            } else if (old_x >= platform->x + platform->width) {
                transform->rect.x = platform->x + platform->width;
            }
            physics->velocity.x = physics->velocity.x * -0.2;
        }
    }

    transform->rect.y = new_y;
    bool landedOnPlatform = false;
    const float grounded_epsilon = 0.5f;
    for (size_t i = 0; i < stage->count; i++) {
        if (CheckCollisionRecs(transform->rect, stage->platforms[i])) {
            const Platform *platform = &stage->platforms[i];
            if (old_y + transform->rect.height <= platform->y + grounded_epsilon && physics->velocity.y >= 0) {
                transform->rect.y = platform->y - transform->rect.height;
                physics->velocity.y = physics->velocity.y * -0.2;
                physics->grounded = true;
                landedOnPlatform = true;
            } else if (old_y >= platform->y + platform->height - grounded_epsilon && physics->velocity.y < 0) {
                transform->rect.y = platform->y + platform->height;
                physics->velocity.y = physics->velocity.y * -0.2;
            }
        }
    }
    if (!landedOnPlatform) {
        bool nearPlatform = false;
        for (size_t i = 0; i < stage->count; i++) {
            const Platform *platform = &stage->platforms[i];
            if (transform->rect.x + transform->rect.width > platform->x &&
                transform->rect.x < platform->x + platform->width &&
                fabs(platform->y - (transform->rect.y + transform->rect.height)) < grounded_epsilon) {
                nearPlatform = true;
                break;
            }
        }
        physics->grounded = nearPlatform;
    }
}
void draw_solid(const TransformComp *transform, const SolidRectangleComp *solid_rectangle) {
    DrawRectangleRec(transform->rect, solid_rectangle->color);
}
