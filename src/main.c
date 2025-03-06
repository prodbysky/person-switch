#include "game_state.h"

int main() {
    GameState state = game_state_init();

    while (!WindowShouldClose()) {
        game_state(&state);
    }
    game_state_destroy(&state);
}
