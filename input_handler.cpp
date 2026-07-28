#include "splashkit.h"
#include "game_state.cpp"

void handle_input(GameState &state)
{
    process_events();

    if (quit_requested())
        return;

    if (state.phase == START_SCREEN)
    {
        if (key_typed(SPACE_KEY))
        {
            reset_gameplay(state);
        }
    }
    else if (state.phase == PLAYING)
    {
        // Handle lane switching
        if (key_typed(LEFT_KEY))
        {
            if (state.player.current_lane == LANE_CENTER)
                state.player.switch_lane(LANE_LEFT);
            else if (state.player.current_lane == LANE_RIGHT)
                state.player.switch_lane(LANE_CENTER);
        }
        else if (key_typed(RIGHT_KEY))
        {
            if (state.player.current_lane == LANE_CENTER)
                state.player.switch_lane(LANE_RIGHT);
            else if (state.player.current_lane == LANE_LEFT)
                state.player.switch_lane(LANE_CENTER);
        }
    }
    else if (state.phase == GAME_OVER)
    {
        if (key_typed(SPACE_KEY))
        {
            reset_gameplay(state);
        }
    }
}
