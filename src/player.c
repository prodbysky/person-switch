#include "player.h"
#include "static_config.h"
#include <raymath.h>

static const PlayerStat PLAYER_STATES[] = {
    [PS_TANK] = {.jump_power = 300, .speed_x = 40},
    [PS_MOVE] = {.jump_power = 600, .speed_x = 70},
    [PS_DAMAGE] = {.jump_power = 400, .speed_x = 50},
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

void player_update(Player *player) {
    if (IsKeyPressed(KEY_F) && GetTime() - player->last_switched > PLAYER_CLASS_SWITCH_COOLDOWN) {
        player->current_class = (player->current_class + 1) % (PS_DAMAGE + 1);
        player->last_switched = GetTime();
    }

    if (IsKeyDown(KEY_A)) {
        player->velocity.x -= PLAYER_STATES[player->current_class].speed_x;
    }
    if (IsKeyDown(KEY_D)) {
        player->velocity.x += PLAYER_STATES[player->current_class].speed_x;
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

    player->rect.x += player->velocity.x * GetFrameTime();
    player->rect.y += player->velocity.y * GetFrameTime();
    player->rect.x = Clamp(player->rect.x, 0, WINDOW_W - player->rect.width);
}
