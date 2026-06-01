#ifndef GAME_TYPES_CPP_INCLUDED
#define GAME_TYPES_CPP_INCLUDED

#include "splashkit.h"

// Screen dimensions
inline constexpr int WINDOW_WIDTH = 1000;
inline constexpr int WINDOW_HEIGHT = 1000;

// Road / lane geometry (centred inside the window)
inline constexpr int ROAD_LEFT = 150;
inline constexpr int ROAD_RIGHT = 650;
inline constexpr int ROAD_WIDTH = ROAD_RIGHT - ROAD_LEFT;

inline constexpr int LANE_WIDTH = ROAD_WIDTH / 3;

// Lane centre X positions (kept for legacy but lane_to_x is canonical)
inline constexpr int LANE_CENTER_LEFT = ROAD_LEFT + LANE_WIDTH / 2;
inline constexpr int LANE_CENTER_CENTER = ROAD_LEFT + LANE_WIDTH + LANE_WIDTH / 2;
inline constexpr int LANE_CENTER_RIGHT = ROAD_LEFT + 2 * LANE_WIDTH + LANE_WIDTH / 2;

// Player car pixel dimensions
inline constexpr int PLAYER_CAR_WIDTH = 124;
inline constexpr int PLAYER_CAR_HEIGHT = 126;
inline constexpr double PLAYER_Y = WINDOW_HEIGHT - 130.0;

// Lane-switch speed (px/frame the player interpolates toward target lane)
inline constexpr double LANE_LERP_SPEED = 12.0;

// Difficulty score thresholds
inline constexpr int LEVEL_2_SCORE = 1200;
inline constexpr int LEVEL_3_SCORE = 5000;

// Maximum number of enemy cars on screen at once
inline constexpr int MAX_ENEMIES = 50;

// Highscore file path (relative to executable)
inline const char *const HIGHSCORE_JSON = "Resources/highscore.json";

// Vehicle dimensions
inline constexpr int SEDAN_WIDTH = 100;
inline constexpr int SEDAN_HEIGHT = 60;
inline constexpr int TRUCK_WIDTH = 140;
inline constexpr int TRUCK_HEIGHT = 80;
inline constexpr int MINIVAN_WIDTH = 110;
inline constexpr int MINIVAN_HEIGHT = 65;
inline constexpr int RECKLESS_WIDTH = 90;
inline constexpr int RECKLESS_HEIGHT = 55;

// Spawn and blockade parameters
inline constexpr int MIN_SPAWN_DELAY = 60;
inline constexpr int MAX_SPAWN_DELAY = 120;
inline constexpr int MIN_LANE_DISTANCE = 150;       // Distance between cars in same lane
inline constexpr int BLOCKADE_CHECK_DISTANCE = 150; // For 3-lane blockade detection

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
inline constexpr int SAFE_WEAVE_GAP = PLAYER_CAR_HEIGHT * 2; // e.g., 2x player height

// Convert a lane enum to an X coordinate on screen, centered regardless of WINDOW_WIDTH
double lane_to_x(Lane lane)
{
    double road_width = static_cast<double>(ROAD_WIDTH);
    double road_left = (static_cast<double>(WINDOW_WIDTH) - road_width) / 2.0;
    double lane_w = road_width / 3.0;
    return road_left + lane_w * static_cast<int>(lane) + lane_w / 2.0;
}

#endif // GAME_TYPES_CPP_INCLUDED
