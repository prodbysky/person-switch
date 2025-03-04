#include "game_state.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>

int main() {
    GameState state = game_state_init_system();

    while (!WindowShouldClose()) {
        game_state_system(&state);
    }
    game_state_destroy(&state);
}
