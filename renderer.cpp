// ---------------------------------------------------------------------------
// renderer.cpp
// Rendering and graphics implementation.
// ---------------------------------------------------------------------------
#include "splashkit.h"
#include "game_state.cpp"
#include <string>

// Forward declarations (functions defined later in this file)
void draw_road(GameState &state);
void draw_start_screen(GameState &state);
void draw_enemies(GameState &state);
void draw_player(PlayerCar &player);
void draw_hud(GameState &state);
void draw_game_over(GameState &state);

// Bitmap globals
bitmap player_bitmap;
bitmap truck_bitmap;
bitmap minivan_bitmap;
bitmap sedan_bitmap;
bitmap reckless_bitmap;

// Road scrolling offset
double road_offset_y = 0.0;

void init_renderer()
{
    open_window("Three-Lane Highway", WINDOW_WIDTH, WINDOW_HEIGHT);

    player_bitmap = load_bitmap("player", "Resources/images/user_car.png");
    truck_bitmap = load_bitmap("truck", "Resources/images/truck.png");
    minivan_bitmap = load_bitmap("minivan", "Resources/images/minivan.png");
    sedan_bitmap = load_bitmap("sedan", "Resources/images/sedan.png");
    reckless_bitmap = load_bitmap("reckless", "Resources/images/reckless_car.png");
}

void cleanup_renderer()
{
    free_bitmap(player_bitmap);
    free_bitmap(truck_bitmap);
    free_bitmap(minivan_bitmap);
    free_bitmap(sedan_bitmap);
    free_bitmap(reckless_bitmap);
}

void draw_game(GameState &state)
{
    clear_screen(COLOR_BLACK);

    if (state.phase == START_SCREEN)
    {
        // Draw road preview behind the start text so start screen is not black
        draw_road(state);
        draw_start_screen(state);
    }
    else
    {
        draw_road(state);
        draw_enemies(state);
        draw_player(state.player);
        draw_hud(state);

        if (state.phase == GAME_OVER)
        {
            draw_game_over(state);
        }
    }

    refresh_screen(60);
}

void draw_road(GameState &state)
{
    // Compute centered road coordinates based on current window width
    double road_w = static_cast<double>(ROAD_WIDTH);
    double road_left = (static_cast<double>(WINDOW_WIDTH) - road_w) / 2.0;
    double road_right = road_left + road_w;

    fill_rectangle(COLOR_GREEN, 0, 0, static_cast<int>(road_left), WINDOW_HEIGHT);
    fill_rectangle(COLOR_GREEN, static_cast<int>(road_right), 0, WINDOW_WIDTH - static_cast<int>(road_right), WINDOW_HEIGHT);

    fill_rectangle(COLOR_DIM_GRAY, static_cast<int>(road_left), 0, static_cast<int>(road_w), WINDOW_HEIGHT);

    if (state.phase == PLAYING)
    {
        road_offset_y += 5.0 * state.global_speed_multiplier;
        if (road_offset_y > 40.0)
            road_offset_y -= 40.0;
    }

    double lane_w = road_w / 3.0;
    for (int y = -40; y < WINDOW_HEIGHT; y += 40)
    {
        fill_rectangle(COLOR_WHITE, static_cast<int>(road_left + lane_w - 2), y + road_offset_y, 4, 20);
        fill_rectangle(COLOR_WHITE, static_cast<int>(road_left + 2 * lane_w - 2), y + road_offset_y, 4, 20);
    }
}

void draw_player(PlayerCar &player)
{
    double draw_x = player.x_pos - bitmap_width(player_bitmap) / 2.0;
    double draw_y = player.y_pos;
    draw_bitmap(player_bitmap, draw_x, draw_y);
}

void draw_enemies(GameState &state)
{
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        bitmap bmp;

        switch (e.type)
        {
        case TRUCK:
            bmp = truck_bitmap;
            break;
        case MINIVAN:
            bmp = minivan_bitmap;
            break;
        case SEDAN:
            bmp = sedan_bitmap;
            break;
        case RECKLESS_CAR:
            bmp = reckless_bitmap;
            break;
        default:
            bmp = sedan_bitmap;
            break;
        }

        double draw_x = e.x_pos - bitmap_width(bmp) / 2.0;
        double draw_y = e.y_pos;
        draw_bitmap(bmp, draw_x, draw_y);
    }
}

void draw_start_screen(GameState &state)
{
    draw_text("THREE-LANE HIGHWAY", COLOR_WHITE, 150, 200);
    draw_text("High Score: " + std::to_string(state.high_score), COLOR_YELLOW, 320, 260);
    draw_text("Press SPACE to Start", COLOR_WHITE, 300, 320);
    draw_text("Controls: LEFT / RIGHT Arrows", COLOR_LIGHT_GRAY, 280, 360);
}

void draw_hud(GameState &state)
{
    fill_rectangle(rgba_color(0, 0, 0, 150), 10, 10, 150, 60);
    draw_text("Score: " + std::to_string(state.score), COLOR_WHITE, 20, 20);
    draw_text("High: " + std::to_string(state.high_score), COLOR_YELLOW, 20, 40);
}

void draw_game_over(GameState &state)
{
    fill_rectangle(rgba_color(0, 0, 0, 200), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    draw_text("CRASHED!", COLOR_RED, 250, 200);
    draw_text("Final Score: " + std::to_string(state.score), COLOR_WHITE, 300, 280);
    draw_text("Press SPACE to Restart", COLOR_WHITE, 280, 330);
}
