#include "player.h"
#include "raylib.h"
#include "stage.h"
#include "static_config.h"
#include <raymath.h>

static const PlayerStat PLAYER_STATES[] = {
    [PS_TANK] = {.jump_power = 300, .speed_x = 200},
    [PS_MOVE] = {.jump_power = 600, .speed_x = 350},
    [PS_DAMAGE] = {.jump_power = 400, .speed_x = 250},
};

Player player_new() {
    return (Player){
        .rect = (Rectangle){.width = 32, .height = 96, .x = (WINDOW_W / 2.0) + 16, .y = (WINDOW_H / 2.0) + 48},
        .velocity = (Vector2){0},
        .current_class = PS_MOVE,
        .grounded = false,
        .last_switched = GetTime(),
    };
}

void player_update(Player *player, const Stage *stage) {
    if (IsKeyPressed(KEY_F) && GetTime() - player->last_switched > PLAYER_CLASS_SWITCH_COOLDOWN) {
        player->current_class = (player->current_class + 1) % (PS_COUNT);
        player->last_switched = GetTime();
    }

    if (IsKeyDown(KEY_A)) {
        player->velocity.x = -PLAYER_STATES[player->current_class].speed_x;
    }
    if (IsKeyDown(KEY_D)) {
        player->velocity.x = PLAYER_STATES[player->current_class].speed_x;
    }

    player->velocity.x /= 1.2;

    if (player->rect.y + player->rect.height > WINDOW_H) {
        player->grounded = true;
    }

    if (!player->grounded) {
        player->velocity.y += G;
        player->velocity.y = Clamp(player->velocity.y, -800, 800);
    } else {
        if (IsKeyDown(KEY_SPACE)) {
            player->velocity.y = -PLAYER_STATES[player->current_class].jump_power;
            player->grounded = false;
        } else {
            player->velocity.y = 0;
        }
    }

    Vector2 next_pos = {
        .x = player->rect.x + player->velocity.x * GetFrameTime(),
        .y = player->rect.y + player->velocity.y * GetFrameTime(),
    };

    bool on_any_platform = false;

    for (int i = 0; i < stage->count; i++) {
        if (CheckCollisionRecs(
                (Rectangle){
                    .x = next_pos.x,
                    .y = next_pos.y,
                    .width = player->rect.width,
                    .height = player->rect.height,
                },
                stage->platforms[i])) {
            const Platform *platform = &stage->platforms[i];
            const bool from_left = next_pos.x < platform->x + platform->width;
            const bool from_right = next_pos.x + player->rect.width > platform->x;
            const bool from_bottom = next_pos.y + player->rect.height > platform->y;

            if (from_bottom && player->velocity.y > 0) {
                player->velocity.y = 0;
                player->grounded = true;
                player->rect.y = platform->y - player->rect.height;
                on_any_platform = true;
            } else if (from_left && player->velocity.x < 0) {
                player->velocity.x = 0;
            } else if (from_right && player->velocity.x > 0) {
                player->velocity.x = 0;
            }
        }
    }

    if (player->grounded && !on_any_platform) {
        bool above_platform = false;
        for (int i = 0; i < stage->count; i++) {
            const Platform *platform = &stage->platforms[i];
            if (player->rect.x + player->rect.width > platform->x && player->rect.x < platform->x + platform->width &&
                player->rect.y + player->rect.height <= platform->y &&
                player->rect.y + player->rect.height + 1 >= platform->y) {
                above_platform = true;
                break;
            }
        }

        if (!above_platform) {
            player->grounded = false;
        }
    }

    next_pos = (Vector2){
        .x = player->rect.x + player->velocity.x * GetFrameTime(),
        .y = player->rect.y + player->velocity.y * GetFrameTime(),
    };

    player->rect.x = next_pos.x;
    player->rect.y = next_pos.y;
    player->rect.x = Clamp(player->rect.x, 0, WINDOW_W - player->rect.width);
}
