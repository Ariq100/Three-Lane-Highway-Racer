#include "splashkit.h"
#include "rng.h"
#include "rng.cpp"
#include "score_io.cpp"
#include "renderer.cpp"
#include "input_handler.cpp"

int main()
{
    init_renderer();

    GameState state;
    initialize_game(state);

    while (!quit_requested())
    {
        handle_input(state);
        update_game(state);
        draw_game(state);
    }

    cleanup_renderer();
    close_all_windows();

    return 0;
}
