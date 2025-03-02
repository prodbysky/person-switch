#include "player.h"
#include "static_config.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>

int main() {
    InitWindow(WINDOW_W, WINDOW_H, "Persona");
    SetTargetFPS(120);
    Player player = player_new();
    while (!WindowShouldClose()) {
        player_update(&player);
        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
        DrawRectangleRec(player.rect, WHITE);
        EndDrawing();
    }
    CloseWindow();
}
