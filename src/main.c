#include <raylib.h>

int main() {
    InitWindow(800, 600, "Persona");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
        EndDrawing();
    }
    CloseWindow();
}
