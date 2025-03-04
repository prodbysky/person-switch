#include "game_state.h"

int main() {
    GameState state = game_state_init_system();

    while (!WindowShouldClose()) {
        game_state_system(&state);
    }
    game_state_destroy(&state);
}
