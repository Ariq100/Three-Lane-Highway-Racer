#ifndef GAME_STATE_CPP_INCLUDED
#define GAME_STATE_CPP_INCLUDED

// ---------------------------------------------------------------------------
// game_state.cpp
// Game state management and logic implementation.
// ---------------------------------------------------------------------------
// This file intentionally contains the GameState definition since headers were removed
#include "game_types.cpp"
#include "dynamic_array.hpp"
#include "rng.h"
#include <vector>
#include <cmath>
#include "player.cpp"
#include "enemy.cpp"

// Forward declarations for score IO (implemented in score_io.cpp)
#include <string>
int load_highscore(const std::string &filepath);
void save_highscore(const std::string &filepath, int highscore);

// GameState struct (previously in header)
struct GameState
{
    GamePhase phase;
    int high_score;
    int score;
    double global_speed_multiplier;
    int spawn_timer;
    PlayerCar player;
    dynamic_array<EnemyCar> active_enemies{10};
};

// Forward declare helpers that are defined later in this file
void remove_enemy(GameState &state, int index);
void handle_collisions(GameState &state);
bool resolve_impasse(GameState &state);
void spawn_enemy(GameState &state);

// ---------------------------------------------------------------------------
// Traffic Management Functions
// ---------------------------------------------------------------------------

// Check if a car can spawn in a lane without overlapping
// Returns: 0 = cannot spawn, 1 = can spawn (no cars), 2 = can spawn (follow speed)
// Sets: *out_speed = speed to use if return is 2
int can_spawn_in_lane(GameState &state, Lane lane, double *out_speed)
{
    int car_count = 0;
    double followed_speed = 0.0;
    double closest_y = -1000.0; // Track the topmost (closest to player) car

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        if (e.current_lane == lane)
        {
            car_count++;
            // Track the closest car to the spawn point (highest y value on screen)
            if (e.y_pos > closest_y)
            {
                closest_y = e.y_pos;
                followed_speed = e.speed;
            }
        }
    }

    // More than one car in this lane - cannot spawn
    if (car_count > 1)
        return 0;

    // No cars in this lane - can spawn freely
    if (car_count == 0)
        return 1;

    // Exactly one car - check distance and return its speed
    if (out_speed != nullptr)
        *out_speed = followed_speed;
    return 2;
}

// Check if a car can safely change lanes
// Parameters: x, y = current position, range_x, range_y = dimensions, target_lane = where it wants to go
// Returns: true if safe to change, false if would collide
Lane x_to_lane(double x_pos)
{
    double dist_left = std::fabs(x_pos - lane_to_x(LANE_LEFT));
    double dist_center = std::fabs(x_pos - lane_to_x(LANE_CENTER));
    double dist_right = std::fabs(x_pos - lane_to_x(LANE_RIGHT));

    if (dist_left <= dist_center && dist_left <= dist_right)
        return LANE_LEFT;
    if (dist_right <= dist_center && dist_right <= dist_left)
        return LANE_RIGHT;
    return LANE_CENTER;
}

bool can_change_lane(GameState &state, double x, double y, int range_x, int range_y, Lane target_lane)
{
    double car_left = x - range_x / 2.0;
    double car_right = x + range_x / 2.0;
    double car_top = y;
    double car_bottom = y + range_y;

    // Block the lane change if it would move into the player's lane and overlap the player.
    {
        double target_x = lane_to_x(target_lane);
        double player_lane_x = lane_to_x(x_to_lane(state.player.x_pos));
        bool enters_player_lane = (std::fabs(target_x - player_lane_x) < 1.0);

        if (enters_player_lane)
        {
            double player_left = state.player.get_left();
            double player_right = state.player.get_right();
            double player_top = state.player.get_top();
            double player_bottom = state.player.get_bottom();

            bool overlap_x = (car_right > player_left) && (car_left < player_right);
            bool overlap_y = (car_bottom > player_top) && (car_top < player_bottom);
            if (overlap_x && overlap_y)
                return false;

            // Prevent merges where the enemy would be too close behind the player
            double gap_behind = car_top - player_bottom; // positive => enemy is below (behind) the player
            if (gap_behind < 220.0)
                return false;

            // Prevent merges where the enemy is slightly ahead and the player would catch up
            double gap_ahead = player_top - car_bottom; // positive => player is behind enemy
            if (gap_ahead >= 0.0 && gap_ahead < 220.0)
                return false;
        }
    }

    // Check all cars in the target lane
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &other = state.active_enemies.get(i);

        // Only check cars in the target lane
        if (other.current_lane != target_lane)
            continue;

        // Check if cars are next to each other (within reasonable distance)
        double vertical_distance = std::fabs(car_top - other.get_bottom());
        if (vertical_distance > 200.0) // Too far away, no collision risk
            continue;

        // Check for spatial overlap
        bool overlap_x = (car_right > other.get_left()) && (car_left < other.get_right());
        bool overlap_y = (car_bottom > other.get_top()) && (car_top < other.get_bottom());

        if (overlap_x && overlap_y)
            return false;

        // Check speed compatibility
        if (other.get_bottom() <= car_top && other.speed < 3.5)
            return false;
        if (other.get_top() >= car_bottom && other.speed > 3.5)
            return false;
    }

    return true; // Safe to change lanes
}

// Count how many cars are currently in the specified lane.
int count_cars_in_lane(GameState &state, Lane lane)
{
    int count = 0;
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        if (state.active_enemies.get(i).current_lane == lane)
            count++;
    }
    return count;
}

// Find the car closest to the bottom of the screen in a lane.
EnemyCar *find_closest_car_in_lane(GameState &state, Lane lane)
{
    EnemyCar *best = nullptr;
    double best_y = -1e9;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        if (e.current_lane == lane && e.y_pos > best_y)
        {
            best_y = e.y_pos;
            best = &e;
        }
    }

    return best;
}

// Return the speed of the car in a lane closest to a given Y coordinate.
double lane_speed_for_reference(GameState &state, Lane lane, double ref_y, double fallback_speed)
{
    EnemyCar *best = nullptr;
    double best_distance = 1e9;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        if (e.current_lane != lane)
            continue;

        double distance = std::fabs(e.y_pos - ref_y);
        if (distance < best_distance)
        {
            best_distance = distance;
            best = &e;
        }
    }

    return best ? best->speed : fallback_speed;
}

// A blocking car can slow down only if there is another car below it in the same lane.
bool can_slow_down_due_to_car_below(EnemyCar &c, GameState &state)
{
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &other = state.active_enemies.get(i);
        if (&other == &c)
            continue;
        if (other.current_lane != c.current_lane)
            continue;

        if (other.y_pos > c.y_pos)
            return true; // there is a car below C
    }
    return false;
}

static double enemy_center_y(EnemyCar &c)
{
    return c.y_pos + c.get_height() / 2.0;
}

static bool lanes_are_adjacent(Lane a, Lane b)
{
    return (a == LANE_LEFT && b == LANE_CENTER) ||
           (a == LANE_CENTER && b == LANE_LEFT) ||
           (a == LANE_CENTER && b == LANE_RIGHT) ||
           (a == LANE_RIGHT && b == LANE_CENTER);
}

static bool adjacent_cars_have_safe_gap(EnemyCar &a, EnemyCar &b, double gap)
{
    return lanes_are_adjacent(a.current_lane, b.current_lane) &&
           std::fabs(enemy_center_y(a) - enemy_center_y(b)) >= gap;
}

static bool adjacent_cars_are_blocking(EnemyCar &a, EnemyCar &b, double gap)
{
    return lanes_are_adjacent(a.current_lane, b.current_lane) &&
           std::fabs(enemy_center_y(a) - enemy_center_y(b)) < gap;
}

bool any_lane_pair_has_gap(GameState &state, double gap)
{
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        for (int j = i + 1; j < state.active_enemies.length(); j++)
        {
            EnemyCar &a = state.active_enemies.get(i);
            EnemyCar &b = state.active_enemies.get(j);
            if (adjacent_cars_have_safe_gap(a, b, gap))
                return true;
        }
    }
    return false;
}

static bool find_blocking_side_cars_for_center(EnemyCar &center, GameState &state, double gap, EnemyCar *&left_blocker, EnemyCar *&right_blocker)
{
    left_blocker = nullptr;
    right_blocker = nullptr;

    if (center.current_lane != LANE_CENTER)
        return false;

    if (center.is_escaping_blockade)
        return false;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &other = state.active_enemies.get(i);
        if (other.current_lane == LANE_LEFT && adjacent_cars_are_blocking(center, other, gap))
        {
            if (left_blocker == nullptr || other.y_pos > left_blocker->y_pos)
                left_blocker = &other;
        }
        else if (other.current_lane == LANE_RIGHT && adjacent_cars_are_blocking(center, other, gap))
        {
            if (right_blocker == nullptr || other.y_pos > right_blocker->y_pos)
                right_blocker = &other;
        }
    }

    return left_blocker != nullptr || right_blocker != nullptr;
}

static bool find_blocked_center_car(GameState &state, double gap, EnemyCar *&center_car, EnemyCar *&left_blocker, EnemyCar *&right_blocker)
{
    center_car = nullptr;
    left_blocker = nullptr;
    right_blocker = nullptr;
    double best_y = -1e9;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &candidate = state.active_enemies.get(i);
        EnemyCar *left = nullptr;
        EnemyCar *right = nullptr;

        if (find_blocking_side_cars_for_center(candidate, state, gap, left, right))
        {
            if (candidate.y_pos > best_y)
            {
                best_y = candidate.y_pos;
                center_car = &candidate;
                left_blocker = left;
                right_blocker = right;
            }
        }
    }

    return center_car != nullptr;
}

static bool has_cleared_blocking_cars(EnemyCar &center, GameState &state, double gap)
{
    double center_top = center.y_pos;
    double safe_window = gap * 1.5;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &other = state.active_enemies.get(i);
        if (other.current_lane != LANE_LEFT && other.current_lane != LANE_RIGHT)
            continue;

        double center_distance = std::fabs(enemy_center_y(center) - enemy_center_y(other));
        if (center_distance > safe_window)
            continue;

        if (center_top < other.get_bottom() + center.get_height())
            return false;
    }

    return true;
}

static bool blockade_state_exists(GameState &state, double gap)
{
    EnemyCar *center_car = nullptr;
    EnemyCar *left_blocker = nullptr;
    EnemyCar *right_blocker = nullptr;
    return find_blocked_center_car(state, gap, center_car, left_blocker, right_blocker);
}

// Attempt to change the blocked center-lane car's lane. Returns true if the change succeeds.
bool attempt_lane_change_for_blocked_car(GameState &state, EnemyCar &car)
{
    if (car.current_lane != LANE_CENTER)
        return false;

    Lane candidates[2] = {LANE_LEFT, LANE_RIGHT};
    for (Lane target_lane : candidates)
    {
        if (can_change_lane(state, car.x_pos, car.y_pos, car.get_width(), car.get_height(), target_lane))
        {
            double matched_speed = lane_speed_for_reference(state, target_lane, car.y_pos, car.speed);
            car.current_lane = target_lane;
            car.target_x = lane_to_x(target_lane);
            car.speed = matched_speed;
            car.awaiting_overtake = false;
            car.overtake_progress = 0.0;
            car.is_escaping_blockade = false;
            car.original_speed = car.speed;
            return true;
        }
    }
    return false;
}

// Prevent 3-lane blockade - check if spawning would block all lanes
// Returns: true if it's safe to spawn, false if it would create a blockade
bool prevent_3lane_blockade(GameState &state, Lane lane, double new_car_width, double new_car_height)
{
    bool has_left = false;
    bool has_center = false;
    bool has_right = false;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        if (e.current_lane == LANE_LEFT)
            has_left = true;
        else if (e.current_lane == LANE_CENTER)
            has_center = true;
        else if (e.current_lane == LANE_RIGHT)
            has_right = true;
    }

    if (!has_left || !has_center || !has_right)
        return true;

    double overtake_gap = PLAYER_CAR_HEIGHT * 1.25;
    return !blockade_state_exists(state, overtake_gap);
}

// ---------------------------------------------------------------------------
// Game Initialization and State Management
// ---------------------------------------------------------------------------

void initialize_game(GameState &state)
{
    state.phase = START_SCREEN;
    state.high_score = load_highscore(HIGHSCORE_JSON);
    state.score = 0;
    state.global_speed_multiplier = 1.0;
    state.spawn_timer = 0;
    state.player = PlayerCar();
    state.active_enemies.clear();
}

void reset_gameplay(GameState &state)
{
    state.phase = PLAYING;
    state.score = 0;
    state.global_speed_multiplier = 1.0;
    state.spawn_timer = 0;
    state.player = PlayerCar();
    state.active_enemies.clear();
}

// ---------------------------------------------------------------------------
// Main Game Loop and Updates
// ---------------------------------------------------------------------------

void update_game(GameState &state)
{
    if (state.phase != PLAYING)
        return;

    if (state.score >= LEVEL_3_SCORE)
        state.global_speed_multiplier = 2.0;
    else if (state.score >= LEVEL_2_SCORE)
        state.global_speed_multiplier = 1.5;
    else
        state.global_speed_multiplier = 1.0;

    state.player.update_position();

    for (int i = state.active_enemies.length() - 1; i >= 0; i--)
    {
        EnemyCar &e = state.active_enemies.get(i);
        e.update_position(state.global_speed_multiplier);

        if (!e.passed_player && e.y_pos > state.player.y_pos + state.player.get_height())
        {
            state.score += 100;
            e.passed_player = true;
        }

        if (e.is_off_screen())
        {
            remove_enemy(state, i);
        }
    }

    handle_collisions(state);
    bool blockade_cleared = resolve_impasse(state);
    if (blockade_cleared)
    {
        spawn_enemy(state);
    }
}

void handle_collisions(GameState &state)
{
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);

        bool overlap_x = (state.player.get_right() > e.get_left()) &&
                         (state.player.get_left() < e.get_right());
        bool overlap_y = (state.player.get_bottom() > e.get_top()) &&
                         (state.player.get_top() < e.get_bottom());

        if (overlap_x && overlap_y)
        {
            state.player.is_crashed = true;
            state.phase = GAME_OVER;

            if (state.score > state.high_score)
            {
                state.high_score = state.score;
                save_highscore(HIGHSCORE_JSON, state.high_score);
            }
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Enemy Spawning and Management
// ---------------------------------------------------------------------------

static bool would_converge_into_blockade(GameState &state, Lane new_lane, double new_speed)
{
    const double CONVERGENCE_ZONE = PLAYER_CAR_HEIGHT * 4.0;
    double front_y[3] = {-1e9, -1e9, -1e9};
    double front_speed[3] = {0.0, 0.0, 0.0};
    bool has_car[3] = {false, false, false};

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        int li = static_cast<int>(e.current_lane);

        if (!has_car[li] || e.y_pos > front_y[li])
        {
            front_y[li] = e.y_pos;
            front_speed[li] = e.speed;
            has_car[li] = true;
        }
    }

    int nl = static_cast<int>(new_lane);
    has_car[nl] = true;
    front_speed[nl] = new_speed;
    double simulated_front = -new_speed * 60.0;
    front_y[nl] = (front_y[nl] > simulated_front ? front_y[nl] : simulated_front);

    if (!has_car[0] || !has_car[1] || !has_car[2])
        return false;

    int pairs[2][2] = {{0, 1}, {1, 2}};
    for (int pair_index = 0; pair_index < 2; pair_index++)
    {
        int a = pairs[pair_index][0];
        int b = pairs[pair_index][1];
        double y_diff = std::fabs(front_y[a] - front_y[b]);
        double spd_diff = std::fabs(front_speed[a] - front_speed[b]);

        if (y_diff < CONVERGENCE_ZONE && spd_diff < 0.5)
            return true;
    }

    return false;
}

void spawn_enemy(GameState &state)
{
    if (state.active_enemies.length() >= state.active_enemies.capacity())
        return;

    int spawn_delay = static_cast<int>(random_int(MIN_SPAWN_DELAY, MAX_SPAWN_DELAY) / state.global_speed_multiplier);

    if (state.spawn_timer < spawn_delay)
    {
        state.spawn_timer++;
        return;
    }
    state.spawn_timer = 0;

    EnemyCar new_enemy;

    // Determine vehicle type (temporarily disable reckless spawns)
    int type_roll = random_int(1, 100);
    if (type_roll <= 20)
        new_enemy.type = TRUCK;
    else if (type_roll <= 45)
        new_enemy.type = MINIVAN;
    else
        new_enemy.type = SEDAN; // RECKLESS disabled for now

    // Set initial speed and behavior
    switch (new_enemy.type)
    {
    case TRUCK:
        new_enemy.behavior_type = CRUISING;
        new_enemy.speed = 2.0;
        break;
    case MINIVAN:
        new_enemy.behavior_type = CRUISING;
        new_enemy.speed = 2.5;
        break;
    case SEDAN:
        new_enemy.behavior_type = OVERTAKING;
        new_enemy.speed = 3.5;
        break;
    case RECKLESS_CAR:
        new_enemy.behavior_type = RECKLESS;
        new_enemy.speed = 3.0; // Slightly slower than sedan
        break;
    }

    // Compute the top-most (newest/top) car Y for each lane to implement SAFE_WEAVE_GAP
    double last_top_y[3];
    for (int i = 0; i < 3; i++)
        last_top_y[i] = 1e9; // large value == no car
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        int li = static_cast<int>(e.current_lane);
        // top is y_pos
        if (e.y_pos < last_top_y[li])
            last_top_y[li] = e.y_pos;
    }

    // Try to spawn in a lane using the new logic
    Lane chosen_lane = LANE_CENTER;
    bool spawned = false;
    int attempts = 0;

    while (!spawned && attempts < 3)
    {
        chosen_lane = static_cast<Lane>(random_int(0, 2));

        // Check if we can spawn in this lane
        double followed_speed = new_enemy.speed;
        int can_spawn_result = can_spawn_in_lane(state, chosen_lane, &followed_speed);

        if (can_spawn_result > 0)
        {
            if (!prevent_3lane_blockade(state, chosen_lane, new_enemy.get_width(), new_enemy.get_height()))
            {
                attempts++;
                continue;
            }

            if (would_converge_into_blockade(state, chosen_lane, new_enemy.speed))
            {
                attempts++;
                continue;
            }

            // If other two lanes both have a recent top car, enforce SAFE_WEAVE_GAP
            int a = (chosen_lane + 1) % 3;
            int b = (chosen_lane + 2) % 3;
            bool both_have = (last_top_y[a] < 1e8) && (last_top_y[b] < 1e8);
            if (both_have)
            {
                double delta = std::fabs(last_top_y[a] - last_top_y[b]);
                if (delta < SAFE_WEAVE_GAP)
                {
                    // The two lanes form an impassable diagonal wall — cannot spawn here
                    attempts++;
                    continue;
                }
            }

            // If following another car (one car present), ensure new car speed is <= followed car
            if (can_spawn_result == 2)
            {
                if (new_enemy.speed <= followed_speed + 0.0001)
                {
                    new_enemy.speed = std::min(new_enemy.speed, followed_speed);
                }
                else
                {
                    // New car would be faster than existing car -> would overlap. Skip this lane.
                    attempts++;
                    continue;
                }
            }

            new_enemy.current_lane = chosen_lane;
            spawned = true;
        }

        attempts++;
    }

    // If we couldn't spawn after 3 attempts, pick any available lane (respecting speed)
    if (!spawned)
    {
        for (int lane_idx = 0; lane_idx < 3 && !spawned; lane_idx++)
        {
            Lane try_lane = static_cast<Lane>(lane_idx);
            double followed_speed = new_enemy.speed;
            int can_spawn_result = can_spawn_in_lane(state, try_lane, &followed_speed);

            if (can_spawn_result > 0 && prevent_3lane_blockade(state, try_lane, new_enemy.get_width(), new_enemy.get_height()))
            {
                if (would_converge_into_blockade(state, try_lane, new_enemy.speed))
                {
                    continue;
                }

                // If single car already there, ensure speed <= followed car
                if (can_spawn_result == 2)
                {
                    if (new_enemy.speed <= followed_speed + 0.0001)
                    {
                        new_enemy.speed = std::min(new_enemy.speed, followed_speed);
                    }
                    else
                    {
                        continue; // cannot spawn here because would be faster
                    }
                }
                new_enemy.current_lane = try_lane;
                spawned = true;
            }
        }
    }

    if (!spawned)
        return;

    // Setup position
    new_enemy.x_pos = lane_to_x(new_enemy.current_lane);
    new_enemy.target_x = new_enemy.x_pos;
    new_enemy.y_pos = -new_enemy.get_height() - 10;

    state.active_enemies.add(new_enemy);
}

void remove_enemy(GameState &state, int index)
{
    state.active_enemies.remove_at(index);
}

// ---------------------------------------------------------------------------
// Impasse Resolution
// ---------------------------------------------------------------------------

bool resolve_impasse(GameState &state)
{
    double overtake_gap = PLAYER_CAR_HEIGHT * 1.25;

    EnemyCar *escaping_center = nullptr;
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e = state.active_enemies.get(i);
        if (e.current_lane == LANE_CENTER && e.is_escaping_blockade)
        {
            escaping_center = &e;
            break;
        }
    }

    if (escaping_center != nullptr)
    {
        escaping_center->escape_timer++;

        if (escaping_center->is_overtaking_to_escape)
        {
            escaping_center->speed = escaping_center->escape_target_speed;

            if (has_cleared_blocking_cars(*escaping_center, state, overtake_gap))
            {
                escaping_center->is_overtaking_to_escape = false;
            }
            return false;
        }

        if (attempt_lane_change_for_blocked_car(state, *escaping_center))
        {
            double matched_speed = 0.0;
            bool found_match = false;
            for (int i = 0; i < state.active_enemies.length(); i++)
            {
                EnemyCar &other = state.active_enemies.get(i);
                if (&other == escaping_center)
                    continue;
                if (other.current_lane == escaping_center->current_lane)
                {
                    matched_speed = other.speed;
                    found_match = true;
                    break;
                }
            }

            escaping_center->speed = found_match ? matched_speed
                                                 : escaping_center->original_speed * 0.15;
            escaping_center->is_escaping_blockade = false;
            escaping_center->is_overtaking_to_escape = false;
            escaping_center->escape_timer = 0;
            return true;
        }

        if (escaping_center->escape_timer > 300)
        {
            escaping_center->current_lane = (escaping_center->current_lane == LANE_CENTER)
                                                ? LANE_LEFT
                                                : escaping_center->current_lane;
            escaping_center->target_x = lane_to_x(escaping_center->current_lane);
            escaping_center->speed = escaping_center->original_speed * 1.5;
            escaping_center->is_escaping_blockade = false;
            escaping_center->is_overtaking_to_escape = false;
            escaping_center->escape_timer = 0;
            return true;
        }

        return false;
    }

    if (!prevent_3lane_blockade(state, LANE_CENTER, 0.0, 0.0))
    {
        EnemyCar *center_car = nullptr;
        EnemyCar *left_blocker = nullptr;
        EnemyCar *right_blocker = nullptr;

        if (find_blocked_center_car(state, overtake_gap, center_car, left_blocker, right_blocker) && center_car != nullptr)
        {
            if (!center_car->is_escaping_blockade)
            {
                center_car->is_escaping_blockade = true;
                center_car->is_overtaking_to_escape = true;
                center_car->original_speed = center_car->speed;
                center_car->escape_timer = 0;

                double player_speed = 3.5; // player forward speed is not modeled explicitly
                center_car->escape_target_speed = static_cast<float>(player_speed * 1.5);
                center_car->speed = center_car->escape_target_speed;
            }
        }

        return false;
    }

    return true;
}

#endif // GAME_STATE_CPP_INCLUDED
