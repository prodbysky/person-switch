#include "ecs.h"
#include "raylib.h"
#include "stage.h"
#include "static_config.h"
#include <raymath.h>

void physics(PhysicsComp *physics, float dt) {
    if (physics->grounded) {
        // Apply ground friction
        physics->velocity.x /= 1 + (5 * dt);
    }
    // Apply air friction
    physics->velocity.x /= (1 + (10 * dt));

    if (!physics->grounded) {
        physics->velocity.y += G * dt;
    } else {
        physics->velocity.y = 0;
    }
    physics->velocity.x = Clamp(physics->velocity.x, -300, 300);
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
    Vector2 next_pos = {transform->rect.x + physics->velocity.x * dt, transform->rect.y + physics->velocity.y * dt};

    bool on_any_platform = false;
    Rectangle next_rect = {next_pos.x, next_pos.y, transform->rect.width, transform->rect.height};

    for (size_t i = 0; i < stage->count; i++) {
        if (CheckCollisionRecs(next_rect, stage->platforms[i])) {
            const Platform *platform = &stage->platforms[i];
            const bool from_left = next_rect.x < platform->x + platform->width;
            const bool from_right = next_rect.x + next_rect.width > platform->x;
            const bool from_bottom = next_rect.y + next_rect.height > platform->y;

            if (from_bottom && physics->velocity.y > 0) {
                physics->velocity.y = 0;
                physics->grounded = true;
                transform->rect.y = platform->y - transform->rect.height;
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
        for (size_t i = 0; i < stage->count; i++) {
            const Platform *platform = &stage->platforms[i];
            if (transform->rect.x + transform->rect.width > platform->x &&
                transform->rect.x < platform->x + platform->width &&
                transform->rect.y + transform->rect.height <= platform->y &&
                transform->rect.y + transform->rect.height + 1 >= platform->y) {
                above_platform = true;
                break;
            }
        }

        if (!above_platform) {
            physics->grounded = false;
        }
    }

    next_pos = (Vector2){
        .x = transform->rect.x + physics->velocity.x * dt,
        .y = transform->rect.y + physics->velocity.y * dt,
    };

    transform->rect.x = next_pos.x;
    transform->rect.y = next_pos.y;

    transform->rect.x = Clamp(transform->rect.x, 0, WINDOW_W - transform->rect.width);
}

void draw_solid(const TransformComp *transform, const SolidRectangleComp *solid_rectangle) {
    DrawRectangleRec(transform->rect, solid_rectangle->color);
}

bool offscreen(const TransformComp *transform) {
    return transform->rect.y > WINDOW_H * 4;
}
