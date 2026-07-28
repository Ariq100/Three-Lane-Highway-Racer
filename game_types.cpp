#ifndef GAME_TYPES_HPP_INCLUDED
#define GAME_TYPES_HPP_INCLUDED

#include "splashkit.h"

// Screen dimensions
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 1000;

// Road/lane geometry (centred inside the window)
const int ROAD_LEFT = 150;
const int ROAD_RIGHT = 650;
const int ROAD_WIDTH = ROAD_RIGHT - ROAD_LEFT;

const int LANE_WIDTH = ROAD_WIDTH / 3;

// const int LANE_CENTER_LEFT = ROAD_LEFT + LANE_WIDTH / 2;
// const int LANE_CENTER_CENTER = ROAD_LEFT + LANE_WIDTH + LANE_WIDTH / 2;
// const int LANE_CENTER_RIGHT = ROAD_LEFT + 2 * LANE_WIDTH + LANE_WIDTH / 2;

// Player car pixel dimensions
const int PLAYER_CAR_WIDTH = 124;
const int PLAYER_CAR_HEIGHT = 126;
const double PLAYER_Y = WINDOW_HEIGHT - 150.0; // players position on the screen

// Lane-switch speed
const double LANE_LERP_SPEED = 12.0;

// Difficulty score thresholds
const int LEVEL_2_SCORE = 1200;
const int LEVEL_3_SCORE = 3000;

// Maximum number of enemy cars on screen at once
const int MAX_ENEMIES = 50;

// Highscore file path (relative to executable)
const char *const HIGHSCORE_JSON = "./highscore.json";

// Vehicle dimensions
const int SEDAN_WIDTH = 120;
const int SEDAN_HEIGHT = 120;
const int TRUCK_WIDTH = 140;
const int TRUCK_HEIGHT = 140;
const int MINIVAN_WIDTH = 124;
const int MINIVAN_HEIGHT = 133;
// const int RECKLESS_WIDTH = 90;
// const int RECKLESS_HEIGHT = 55;

// Spawn and blockade parameters
const int MIN_SPAWN_DELAY = 60;
const int MAX_SPAWN_DELAY = 120;
const int MIN_LANE_DISTANCE = 200;       // Distance between cars in same lane
const int BLOCKADE_CHECK_DISTANCE = 200; // For 3-lane blockade detection

enum GamePhase
{
    START_SCREEN,
    PLAYING,
    GAME_OVER
};

enum Lane
{
    LANE_LEFT = 0,
    LANE_CENTER = 1,
    LANE_RIGHT = 2
};

enum VehicleType
{
    TRUCK,
    MINIVAN,
    SEDAN,
    RECKLESS_CAR
};

enum BehaviorType
{
    CRUISING,
    OVERTAKING,
    RECKLESS
};

// Safe weave distance (player needs this vertical gap to weave diagonally)
const int SAFE_WEAVE_GAP = PLAYER_CAR_HEIGHT * 2;

// Convert a lane enum to an X coordinate on screen, centered regardless of WINDOW_WIDTH
double lane_to_x(Lane lane)
{
    double road_width = static_cast<double>(ROAD_WIDTH);
    double road_left = (static_cast<double>(WINDOW_WIDTH) - road_width) / 2.0;
    double lane_w = road_width / 3.0;
    return road_left + lane_w * static_cast<int>(lane) + lane_w / 2.0;
}

#endif