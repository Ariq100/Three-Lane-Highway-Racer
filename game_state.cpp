// Used to prevent redifinition
#ifndef GAME_STATE_CPP_INCLUDED
#define GAME_STATE_CPP_INCLUDED

#include "game_types.cpp"
#include "dynamic_array.hpp"
#include "rng.h"
#include <cmath>
#include "player.cpp"
#include "enemy.cpp"

// Forward declarations for score IO
#include <string>
using std::string;

int load_highscore(const string &filepath);
void save_highscore(const string &filepath, int highscore);

// GameState struct
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
void apply_player_slipstream_effect(GameState &state);

// Traffic Management Functions:

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
        EnemyCar &enemy = state.active_enemies.get(i);
        if (enemy.current_lane == lane)
        {
            car_count++;
            // Track the closest car to the spawn point (highest y value on screen)
            if (enemy.y_pos > closest_y)
            {
                closest_y = enemy.y_pos;
                followed_speed = enemy.speed;
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

// Checks if a car can safely change lanes
// Parameters: x, y = current position, range_x, range_y = dimensions of the car, target_lane = where it wants to go
// Returns: true if safe to change, false if would collide
Lane x_to_lane(double x_pos)
{
    double dist_left = fabs(x_pos - lane_to_x(LANE_LEFT));
    double dist_center = fabs(x_pos - lane_to_x(LANE_CENTER));
    double dist_right = fabs(x_pos - lane_to_x(LANE_RIGHT));

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

        // Prevent merges where the enemy is slightly ahead and the player would catch up.
        // Allow escaping blockade cars to change lanes immediately once they are ahead.
        bool escaping_blockade_exemption = false;
        for (int i = 0; i < state.active_enemies.length(); i++)
        {
            EnemyCar &enemy = state.active_enemies.get(i);
            if (enemy.is_escaping_blockade && enemy.current_lane == x_to_lane(x) &&
                std::fabs(enemy.x_pos - x) < 1.0 && std::fabs(enemy.y_pos - y) < 1.0)
            {
                escaping_blockade_exemption = true;
                break;
            }
        }

        double gap_ahead = player_top - car_bottom; // positive => player is behind enemy
        if (gap_ahead >= 0.0 && gap_ahead < 220.0 && !escaping_blockade_exemption)
            return false;
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
    EnemyCar *best = nullptr; // this variable is used to store the car that was at the bottom

    double best_y = -1000000000.0; // Want the maximum y_pos (closest to bottom)

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);
        if (e_car.current_lane == lane && e_car.y_pos > best_y)
        {
            best_y = e_car.y_pos;
            best = &e_car;
        }
    }

    return best;
}

// Return the speed of the car in a lane closest to a given Y coordinate.
double lane_speed_for_reference(GameState &state, Lane lane, double ref_y, double fallback_speed)
{
    // Prefer the car that is directly ahead of ref_y in this lane —
    // i.e. the car with the highest y_pos that is still less than ref_y
    // (lower on screen = further ahead in the direction of travel).
    // If no car is ahead, fall back to the closest car behind.
    EnemyCar *best_ahead  = nullptr;
    double    best_ahead_y = -1000000000.0; // want the largest y that is still < ref_y

    EnemyCar *best_behind = nullptr;
    double    best_behind_dist = 1000000000.0;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &enemy = state.active_enemies.get(i);
        if (enemy.current_lane != lane)
            continue;

        if (enemy.y_pos < ref_y)
        {
            double ahead_y = enemy.y_pos;
            if (best_ahead == nullptr || ahead_y > best_ahead_y)
            {
                best_ahead_y = ahead_y;
                best_ahead = &enemy;
            }
        }
        else
        {
            // Car is behind reference position — keep as fallback
            double dist = enemy.y_pos - ref_y;
            if (dist < best_behind_dist)
            {
                best_behind_dist = dist;
                best_behind = &enemy;
            }
        }
    }

    if (best_ahead != nullptr)
        return best_ahead->speed;
    if (best_behind != nullptr)
        return best_behind->speed;
    return fallback_speed;
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

double enemy_center_y(EnemyCar &c)
{
    return c.y_pos + c.get_height() / 2.0;
}

bool lanes_are_adjacent(Lane a, Lane b)
{
    return (a == LANE_LEFT && b == LANE_CENTER) ||
           (a == LANE_CENTER && b == LANE_LEFT) ||
           (a == LANE_CENTER && b == LANE_RIGHT) ||
           (a == LANE_RIGHT && b == LANE_CENTER);
}

// function to check if the user does not get boxed in
bool adjacent_cars_have_safe_gap(EnemyCar &a, EnemyCar &b, double gap)
{
    return lanes_are_adjacent(a.current_lane, b.current_lane) && fabs(enemy_center_y(a) - enemy_center_y(b)) >= gap;
}

bool adjacent_cars_are_blocking(EnemyCar &a, EnemyCar &b, double gap)
{
    return lanes_are_adjacent(a.current_lane, b.current_lane) && fabs(enemy_center_y(a) - enemy_center_y(b)) < gap;
}

// Question? Does this function serve any purpose? It seems to just check if there are any two cars with a safe gap, but it doesn't check for a specific lane or car.
bool any_lane_pair_has_gap(GameState &state, double gap)
{
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        for (int j = i + 1; j < state.active_enemies.length(); j++)
        {
            EnemyCar a = state.active_enemies.get(i);
            EnemyCar b = state.active_enemies.get(j);
            if (adjacent_cars_have_safe_gap(a, b, gap))
                return true;
        }
    }
    return false;
}

bool find_blocking_side_cars_for_center(EnemyCar &center, GameState &state, double gap, EnemyCar *&left_blocker, EnemyCar *&right_blocker)
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

    return left_blocker != nullptr && right_blocker != nullptr;
}

bool find_blocked_center_car(GameState &state, double gap, EnemyCar *&center_car, EnemyCar *&left_blocker, EnemyCar *&right_blocker)
{
    center_car = nullptr;
    left_blocker = nullptr;
    right_blocker = nullptr;
    double best_y = -1000000000.0;

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

bool has_cleared_blocking_cars(EnemyCar &center, GameState &state, double gap)
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

bool blockade_state_exists(GameState &state, double gap)
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

// checks if spawning would block all lanes
// Prevents 3-lane blockade if detected
// Parameters:
// - state: current game state
// - lane: candidate lane for a new spawn
// - new_car_width/new_car_height: size of the car being spawned (unused for now)
// Returns: true if spawning in `lane` is considered safe (won't form full blockade)
// This function checks whether all three lanes are occupied and, if so,
// whether the current formation would prevent overtaking (blockade_state_exists).
bool prevent_3lane_blockade(GameState &state, Lane lane, double new_car_width, double new_car_height)
{
    bool has_left = false;
    bool has_center = false;
    bool has_right = false;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &enemy = state.active_enemies.get(i);
        if (enemy.current_lane == LANE_LEFT)
            has_left = true;
        else if (enemy.current_lane == LANE_CENTER)
            has_center = true;
        else if (enemy.current_lane == LANE_RIGHT)
            has_right = true;
    }

    if (!has_left || !has_center || !has_right)
        return true;

    double overtake_gap = PLAYER_CAR_HEIGHT * 1.25;
    return !blockade_state_exists(state, overtake_gap);
}

// initialize_game
// Set up the initial `GameState` when the program launches. Loads highscore and clears active enemies and player state.
// Parameter: state - state object to initialize
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

// reset_gameplay
// Reset in-play values to start a fresh run while preserving persistent data.
// Parameter: state - the GameState to reset into PLAYING mode
void reset_gameplay(GameState &state)
{
    state.phase = PLAYING;
    state.score = 0;
    state.global_speed_multiplier = 1.0;
    state.spawn_timer = 0;
    state.player = PlayerCar();
    state.active_enemies.clear();
}

// Main Game Loop and Updates

// update_game
// Handles difficulty scaling, scoring, collision checks, and impasse resolution.
// Parameter: state - current game state
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
        EnemyCar &e_car = state.active_enemies.get(i);
        e_car.update_position(state.global_speed_multiplier);

        if (!e_car.passed_player && e_car.y_pos > state.player.y_pos + state.player.get_height())
        {
            state.score += 100;
            e_car.passed_player = true;
        }

        if (e_car.is_off_screen())
        {
            remove_enemy(state, i);
        }
    }

    apply_player_slipstream_effect(state);
    handle_collisions(state);
    bool blockade_cleared = resolve_impasse(state);
    if (blockade_cleared)
    {
        spawn_enemy(state);
    }
}

// handle_collisions: Check collisions between the player and every active enemy car. Uses AABB (axis-aligned bounding box) overlap tests on each enemy and updates `state.phase` to `GAME_OVER` and records a new high score if applicable.
// Parameter: state - current game state
void handle_collisions(GameState &state)
{
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);

        bool overlap_x = (state.player.get_right() > e_car.get_left()) && (state.player.get_left() < e_car.get_right());
        bool overlap_y = (state.player.get_bottom() > e_car.get_top()) && (state.player.get_top() < e_car.get_bottom());

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

// Apply slipstream effect: center-lane cars ahead of the player slow down to 50% of player speed
// This scales dynamically with the level
void apply_player_slipstream_effect(GameState &state)
{
    double player_speed = 3.5 * state.global_speed_multiplier;
    double target_speed = player_speed * 0.5;

    // How close a side-lane car must be (vertically) to the center car
    // to be considered "blocking" that lane alongside it.
    const double BLOCK_WINDOW = PLAYER_CAR_HEIGHT * 2.5;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);

        // Only affect center-lane cars
        if (e_car.current_lane != LANE_CENTER)
            continue;

        bool left_blocked = false;
        bool right_blocked = false;

        // Determine whether both side lanes are blocked near this center car.
        for (int j = 0; j < state.active_enemies.length(); j++)
        {
            EnemyCar &other = state.active_enemies.get(j);
            if (other.current_lane == LANE_CENTER)
                continue;

            double vertical_distance = fabs(other.get_top() - e_car.get_top());
            if (vertical_distance > BLOCK_WINDOW)
                continue;

            if (other.current_lane == LANE_LEFT)
                left_blocked = true;
            if (other.current_lane == LANE_RIGHT)
                right_blocked = true;
        }

        // Only apply slipstream if BOTH side lanes are occupied near this car
        if (left_blocked && right_blocked)
        {
            e_car.speed = target_speed;
        }
    }
}

// Enemy Spawning and Management
// This function checks if spawnin a new car at a particular lane with a particular speed will block all three lanes or not
const double CONVERGENCE_ZONE = PLAYER_CAR_HEIGHT * 4.0;

bool would_converge_into_blockade(GameState &state, Lane new_lane, double new_speed)
{
    double front_y[3] = {-1000000000.0, -1000000000.0, -1000000000.0};
    double front_speed[3] = {0.0, 0.0, 0.0};
    bool has_car[3] = {false, false, false};

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);
        int li = static_cast<int>(e_car.current_lane);

        if (!has_car[li] || e_car.y_pos > front_y[li])
        {
            front_y[li] = e_car.y_pos;
            front_speed[li] = e_car.speed;
            has_car[li] = true;
        }
    }

    int new_l = new_lane;
    has_car[new_l] = true;
    front_speed[new_l] = new_speed;
    double simulated_front = -new_speed * 60.0;
    front_y[new_l] = (front_y[new_l] > simulated_front ? front_y[new_l] : simulated_front);

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

// spawn_enemy
// Attempt to create and place a new EnemyCar into the world.
// Steps:
// - respect spawn delay (`spawn_timer`)
// - choose a vehicle type and base speed
// - determine a lane that does not cause immediate overlap or blockade
// - set initial position just above the top of the screen and add to active_enemies
// Parameter: state - current game state
void spawn_enemy(GameState &state)
{
    if (state.active_enemies.length() >= state.active_enemies.capacity())
        return;

    int spawn_delay = (random_int(MIN_SPAWN_DELAY, MAX_SPAWN_DELAY) / state.global_speed_multiplier);

    if (state.spawn_timer < spawn_delay)
    {
        state.spawn_timer++;
        return;
    }

    state.spawn_timer = 1;

    EnemyCar new_enemy;

    // Determine vehicle type
    int type_roll = static_cast<int>(random_int(1, 100));
    if (type_roll <= 20)
        new_enemy.type = TRUCK;
    else if (type_roll <= 45)
        new_enemy.type = MINIVAN;
    else
        new_enemy.type = SEDAN;

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
        new_enemy.speed = 3.95;
        break;
    }

    // Compute the top-most (newest/top) car Y for each lane to implement SAFE_WEAVE_GAP
    double last_top_y[3];

    for (int i = 0; i < 3; i++)
        last_top_y[i] = 1000000000.0; // large value == no car
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);
        int lane_of_e_car = e_car.current_lane;
        // top is y_pos
        if (e_car.y_pos < last_top_y[lane_of_e_car])
            last_top_y[lane_of_e_car] = e_car.y_pos;
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
            bool both_have = (last_top_y[a] < 100000000.0) && (last_top_y[b] < 100000000.0);
            if (both_have)
            {
                double delta = fabs(last_top_y[a] - last_top_y[b]);
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

// Remove an enemy by index from the active_enemies list. Parameter: state, index
void remove_enemy(GameState &state, int index)
{
    state.active_enemies.remove_at(index);
}

// Impasse Resolution

// Attempt to resolve traffic impasses (three-lane blockades) by:
// - letting an `is_escaping_blockade` center car manage its escape
// - attempting lane-changes for blocked cars
// - forcing simple fallback maneuvers when timers expire
// Returns: true if the impasse was resolved and a spawn can proceed
bool resolve_impasse(GameState &state)
{
    double overtake_gap = PLAYER_CAR_HEIGHT * 1.25;

    EnemyCar *escaping_center = nullptr;
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);
        if (e_car.current_lane == LANE_CENTER && e_car.is_escaping_blockade)
        {
            escaping_center = &e_car;
            break;
        }
    }

    if (escaping_center != nullptr)
    {
        escaping_center->escape_timer++;

        if (escaping_center->is_overtaking_to_escape)
        {
            // Re-evaluate every frame whether the player is still behind us in center lane.
            // Once the player has overtaken us or moved out of center, switch to boost speed.
            Lane player_lane = x_to_lane(state.player.x_pos);
            bool player_in_center = (player_lane == LANE_CENTER);
            bool player_behind_center = (state.player.get_top() > escaping_center->get_bottom());

            if (player_in_center && player_behind_center)
            {
                // Still blocked by player — hold slow speed
                escaping_center->escape_target_speed = (3.5 * state.global_speed_multiplier * 0.001);
                escaping_center->speed = escaping_center->escape_target_speed;
            }
            else
            {
                // Player has passed or is not in center
                escaping_center->escape_target_speed = 3.5 * 0.95; // I might need to change this
                escaping_center->speed = escaping_center->escape_target_speed;
            }

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

            escaping_center->speed = found_match ? matched_speed : escaping_center->original_speed * 1.5 * state.global_speed_multiplier;
            escaping_center->is_escaping_blockade = false;
            escaping_center->is_overtaking_to_escape = false;
            escaping_center->escape_timer = 0;
            return true;
        }

        if (escaping_center->escape_timer > 150)
        {
            escaping_center->current_lane = (escaping_center->current_lane == LANE_CENTER) ? LANE_LEFT : escaping_center->current_lane;
            escaping_center->target_x = lane_to_x(escaping_center->current_lane);
            escaping_center->speed = escaping_center->original_speed * 1.5 * state.global_speed_multiplier;
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

                // Check if the player is in the center lane and behind this enemy car.
                // "Behind" means the player's top edge is below the enemy's bottom edge (higher y_pos).
                Lane player_lane = x_to_lane(state.player.x_pos);
                bool player_in_center = (player_lane == LANE_CENTER);
                bool player_behind_center = (state.player.get_top() > center_car->get_bottom());

                if (player_in_center && player_behind_center)
                {
                    // Player is directly behind — slow down
                    // then we overtake the blockers
                    center_car->escape_target_speed = 3.5 * state.global_speed_multiplier * 0.001;
                    center_car->speed = center_car->escape_target_speed;
                }
                else
                {
                    // Player is not behind us in center — safe to slow down and overtake
                    center_car->escape_target_speed = 3.5 * 2 * state.global_speed_multiplier;
                    center_car->speed = center_car->escape_target_speed;
                }
            }
        }

        return false;
    }

    return true;
}

#endif // GAME_STATE_CPP_INCLUDED
