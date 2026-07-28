// consolidated.cpp
// Combined source file for assessment: all primary game code and test suite.

#include "splashkit.h"
#include <cmath>
#include <string>
#include <iostream>
#include <cassert>
#include <fstream>
#include <algorithm>
#include <stdexcept>

// ---------------------------------------------------------------------------
// dynamic_array.hpp
// ---------------------------------------------------------------------------

template <typename T>
class dynamic_array
{
private:
    int size;
    int current_capacity;
    T *data;

    void resize()
    {
        current_capacity *= 2;
        T *new_data = new T[current_capacity];
        for (int i = 0; i < size; i++)
        {
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
    }

public:
    dynamic_array(int initial_capacity = 10)
    {
        size = 0;
        current_capacity = initial_capacity;
        data = new T[current_capacity];
    }

    ~dynamic_array()
    {
        delete[] data;
    }

    dynamic_array(const dynamic_array &other)
    {
        size = other.size;
        current_capacity = other.current_capacity;
        data = new T[current_capacity];
        for (int i = 0; i < size; i++)
        {
            data[i] = other.data[i];
        }
    }

    dynamic_array &operator=(const dynamic_array &other)
    {
        if (this != &other)
        {
            delete[] data;
            size = other.size;
            current_capacity = other.current_capacity;
            data = new T[current_capacity];
            for (int i = 0; i < size; i++)
            {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    void add(T value)
    {
        if (size == current_capacity)
        {
            resize();
        }
        data[size] = value;
        size++;
    }

    int length() const
    {
        return size;
    }

    int capacity() const
    {
        return current_capacity;
    }

    int remove_element(int index)
    {
        if (index >= 0 && index < size)
        {
            for (int i = index; i < size - 1; i++)
            {
                data[i] = data[i + 1];
            }
            size--;
            return index;
        }
        else
        {
            throw std::out_of_range("Failed to remove element: index is out of bounds.");
        }
    }

    int remove_at(int index) { return remove_element(index); }

    void clear() { size = 0; }

    void fill(T value)
    {
        for (int i = 0; i < current_capacity; i++)
        {
            data[i] = value;
        }
        size = current_capacity;
    }

    T &get(int index)
    {
        if (index >= 0 && index < size)
        {
            return data[index];
        }
        else
        {
            throw std::out_of_range("Failed to get value: index is out of bounds.");
        }
    }

    const T &get(int index) const
    {
        if (index >= 0 && index < size)
        {
            return data[index];
        }
        else
        {
            throw std::out_of_range("Failed to get value: index is out of bounds.");
        }
    }

    T &operator[](int index)
    {
        return get(index);
    }

    const T &operator[](int index) const
    {
        return get(index);
    }
};

// ---------------------------------------------------------------------------
// game_types.cpp
// ---------------------------------------------------------------------------

// Screen dimensions
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 1000;

// Road/lane geometry (centred inside the window)
const int ROAD_LEFT = 150;
const int ROAD_RIGHT = 650;
const int ROAD_WIDTH = ROAD_RIGHT - ROAD_LEFT;

const int LANE_WIDTH = ROAD_WIDTH / 3;

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

// ---------------------------------------------------------------------------
// rng.h + rng.cpp
// ---------------------------------------------------------------------------

// Returns a random integer between min and max (inclusive)
int random_int(int min, int max);

// Returns a random double between min and max
double random_double(double min, double max);

int random_int(int min, int max)
{
    return min + rnd(max - min + 1);
}

double random_double(double min, double max)
{
    return min + rnd() * (max - min);
}

// ---------------------------------------------------------------------------
// player.cpp
// ---------------------------------------------------------------------------

class PlayerCar
{
public:
    Lane current_lane;
    Lane target_lane;
    double x_pos;
    double target_x;
    double y_pos;
    bool is_crashed;

    PlayerCar()
    {
        current_lane = LANE_CENTER;
        target_lane = LANE_CENTER;
        x_pos = lane_to_x(LANE_CENTER);
        target_x = x_pos;
        y_pos = PLAYER_Y;
        is_crashed = false;
    }

    void switch_lane(Lane new_lane)
    {
        if (is_crashed)
            return;

        if (new_lane == LANE_LEFT && current_lane == LANE_LEFT)
            return;
        if (new_lane == LANE_RIGHT && current_lane == LANE_RIGHT)
            return;

        target_lane = new_lane;
        target_x = lane_to_x(new_lane);
        current_lane = new_lane;
    }

    void update_position()
    {
        double diff = target_x - x_pos;

        if (std::fabs(diff) <= LANE_LERP_SPEED)
            x_pos = target_x;
        else
            x_pos += (diff > 0 ? LANE_LERP_SPEED : -LANE_LERP_SPEED);
    }

    int get_width()
    {
        return PLAYER_CAR_WIDTH;
    }

    int get_height()
    {
        return PLAYER_CAR_HEIGHT;
    }

    double get_left()
    {
        return x_pos - PLAYER_CAR_WIDTH / 2.0;
    }

    double get_right()
    {
        return x_pos + PLAYER_CAR_WIDTH / 2.0;
    }

    double get_top()
    {
        return y_pos;
    }

    double get_bottom()
    {
        return y_pos + PLAYER_CAR_HEIGHT;
    }
};

// ---------------------------------------------------------------------------
// enemy.cpp
// ---------------------------------------------------------------------------

class EnemyCar
{
public:
    VehicleType type;
    BehaviorType behavior_type;
    Lane current_lane;
    double x_pos;
    double y_pos;
    double target_x;
    double speed;
    bool passed_player;
    int frames_alive;
    int lane_change_timer;
    bool awaiting_overtake;
    double overtake_progress;
    bool is_escaping_blockade;
    double original_speed;
    int escape_timer;
    bool is_overtaking_to_escape;
    float escape_target_speed;

    EnemyCar()
    {
        type = SEDAN;
        behavior_type = CRUISING;
        current_lane = LANE_CENTER;
        x_pos = lane_to_x(LANE_CENTER);
        y_pos = -100.0;
        target_x = lane_to_x(LANE_CENTER);
        speed = 3.0;
        passed_player = false;
        frames_alive = 0;
        lane_change_timer = 0;
        awaiting_overtake = false;
        overtake_progress = 0.0;
        is_escaping_blockade = false;
        original_speed = speed;
        escape_timer = 0;
        is_overtaking_to_escape = false;
        escape_target_speed = 0.0f;
    }

    void update_position(double global_multiplier)
    {
        frames_alive++;
        if (lane_change_timer > 0)
            lane_change_timer--;

        double travel = speed * global_multiplier;
        y_pos += travel;

        if (awaiting_overtake)
        {
            overtake_progress += std::fabs(travel);
        }

        double diff = target_x - x_pos;
        if (std::fabs(diff) > 2.0)
            x_pos += diff * 0.08;
        else
            x_pos = target_x;
    }

    bool is_off_screen()
    {
        return y_pos > static_cast<double>(WINDOW_HEIGHT + get_height());
    }

    int get_width()
    {
        switch (type)
        {
        case TRUCK:
            return TRUCK_WIDTH;
        case MINIVAN:
            return MINIVAN_WIDTH;
        default:
            return SEDAN_WIDTH;
        }
    }

    int get_height()
    {
        switch (type)
        {
        case TRUCK:
            return TRUCK_HEIGHT;
        case MINIVAN:
            return MINIVAN_HEIGHT;
        default:
            return SEDAN_HEIGHT;
        }
    }

    double get_left()
    {
        return x_pos - get_width() / 2.0;
    }

    double get_right()
    {
        return x_pos + get_width() / 2.0;
    }

    double get_top()
    {
        return y_pos;
    }

    double get_bottom()
    {
        return y_pos + get_height();
    }
};

// ---------------------------------------------------------------------------
// score_io.cpp
// ---------------------------------------------------------------------------

int load_highscore(const std::string &filepath)
{
    int highscore = 0;

    json j = json_from_file(filepath);
    if (json_has_key(j, "highscore"))
    {
        highscore = json_read_number_as_int(j, "highscore");
    }
    free_json(j);

    return highscore;
}

void save_highscore(const std::string &filepath, int highscore)
{
    json j = create_json();
    json_set_number(j, "highscore", highscore);
    json_to_file(j, filepath);
    free_json(j);
}

// ---------------------------------------------------------------------------
// game_state.cpp
// ---------------------------------------------------------------------------

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

void remove_enemy(GameState &state, int index);
void handle_collisions(GameState &state);
bool resolve_impasse(GameState &state);
void spawn_enemy(GameState &state);
void apply_player_slipstream_effect(GameState &state);

int can_spawn_in_lane(GameState &state, Lane lane, double *out_speed)
{
    int car_count = 0;
    double followed_speed = 0.0;
    double closest_y = -1000.0;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &enemy = state.active_enemies.get(i);
        if (enemy.current_lane == lane)
        {
            car_count++;
            if (enemy.y_pos > closest_y)
            {
                closest_y = enemy.y_pos;
                followed_speed = enemy.speed;
            }
        }
    }

    if (car_count > 1)
        return 0;

    if (car_count == 0)
        return 1;

    if (out_speed != nullptr)
        *out_speed = followed_speed;
    return 2;
}

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

        double gap_behind = car_top - player_bottom;
        if (gap_behind < 220.0)
            return false;

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

        double gap_ahead = player_top - car_bottom;
        if (gap_ahead >= 0.0 && gap_ahead < 220.0 && !escaping_blockade_exemption)
            return false;
    }

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &other = state.active_enemies.get(i);
        if (other.current_lane != target_lane)
            continue;

        double vertical_distance = std::fabs(car_top - other.get_bottom());
        if (vertical_distance > 200.0)
            continue;

        bool overlap_x = (car_right > other.get_left()) && (car_left < other.get_right());
        bool overlap_y = (car_bottom > other.get_top()) && (car_top < other.get_bottom());

        if (overlap_x && overlap_y)
            return false;

        if (other.get_bottom() <= car_top && other.speed < 3.5)
            return false;
        if (other.get_top() >= car_bottom && other.speed > 3.5)
            return false;
    }

    return true;
}

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

EnemyCar *find_closest_car_in_lane(GameState &state, Lane lane)
{
    EnemyCar *best = nullptr;
    double best_y = -1000000000.0;

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

double lane_speed_for_reference(GameState &state, Lane lane, double ref_y, double fallback_speed)
{
    EnemyCar *best_ahead = nullptr;
    double best_ahead_y = -1000000000.0;

    EnemyCar *best_behind = nullptr;
    double best_behind_dist = 1000000000.0;

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
            return true;
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

bool adjacent_cars_have_safe_gap(EnemyCar &a, EnemyCar &b, double gap)
{
    return lanes_are_adjacent(a.current_lane, b.current_lane) && fabs(enemy_center_y(a) - enemy_center_y(b)) >= gap;
}

bool adjacent_cars_are_blocking(EnemyCar &a, EnemyCar &b, double gap)
{
    return lanes_are_adjacent(a.current_lane, b.current_lane) && fabs(enemy_center_y(a) - enemy_center_y(b)) < gap;
}

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

void apply_player_slipstream_effect(GameState &state)
{
    double player_speed = 3.5 * state.global_speed_multiplier;
    double target_speed = player_speed * 0.5;
    const double BLOCK_WINDOW = PLAYER_CAR_HEIGHT * 2.5;

    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);
        if (e_car.current_lane != LANE_CENTER)
            continue;

        bool left_blocked = false;
        bool right_blocked = false;

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

        if (left_blocked && right_blocked)
        {
            e_car.speed = target_speed;
        }
    }
}

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

    int type_roll = static_cast<int>(random_int(1, 100));
    if (type_roll <= 20)
        new_enemy.type = TRUCK;
    else if (type_roll <= 45)
        new_enemy.type = MINIVAN;
    else
        new_enemy.type = SEDAN;

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
    case RECKLESS_CAR:
        new_enemy.behavior_type = RECKLESS;
        new_enemy.speed = 4.5;
        break;
    }

    double last_top_y[3];
    for (int i = 0; i < 3; i++)
        last_top_y[i] = 1000000000.0;
    for (int i = 0; i < state.active_enemies.length(); i++)
    {
        EnemyCar &e_car = state.active_enemies.get(i);
        int lane_of_e_car = e_car.current_lane;
        if (e_car.y_pos < last_top_y[lane_of_e_car])
            last_top_y[lane_of_e_car] = e_car.y_pos;
    }

    Lane chosen_lane = LANE_CENTER;
    bool spawned = false;
    int attempts = 0;

    while (!spawned && attempts < 3)
    {
        chosen_lane = static_cast<Lane>(random_int(0, 2));

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

            int a = (chosen_lane + 1) % 3;
            int b = (chosen_lane + 2) % 3;
            bool both_have = (last_top_y[a] < 100000000.0) && (last_top_y[b] < 100000000.0);
            if (both_have)
            {
                double delta = fabs(last_top_y[a] - last_top_y[b]);
                if (delta < SAFE_WEAVE_GAP)
                {
                    attempts++;
                    continue;
                }
            }

            if (can_spawn_result == 2)
            {
                if (new_enemy.speed <= followed_speed + 0.0001)
                {
                    new_enemy.speed = std::min(new_enemy.speed, followed_speed);
                }
                else
                {
                    attempts++;
                    continue;
                }
            }

            new_enemy.current_lane = chosen_lane;
            spawned = true;
        }

        attempts++;
    }

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

                if (can_spawn_result == 2)
                {
                    if (new_enemy.speed <= followed_speed + 0.0001)
                    {
                        new_enemy.speed = std::min(new_enemy.speed, followed_speed);
                    }
                    else
                    {
                        continue;
                    }
                }
                new_enemy.current_lane = try_lane;
                spawned = true;
            }
        }
    }

    if (!spawned)
        return;

    new_enemy.x_pos = lane_to_x(new_enemy.current_lane);
    new_enemy.target_x = new_enemy.x_pos;
    new_enemy.y_pos = -new_enemy.get_height() - 10;

    state.active_enemies.add(new_enemy);
}

void remove_enemy(GameState &state, int index)
{
    state.active_enemies.remove_at(index);
}

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
            Lane player_lane = x_to_lane(state.player.x_pos);
            bool player_in_center = (player_lane == LANE_CENTER);
            bool player_behind_center = (state.player.get_top() > escaping_center->get_bottom());

            if (player_in_center && player_behind_center)
            {
                escaping_center->escape_target_speed = (3.5 * state.global_speed_multiplier * 0.001);
                escaping_center->speed = escaping_center->escape_target_speed;
            }
            else
            {
                escaping_center->escape_target_speed = 3.5 * 0.95;
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

                Lane player_lane = x_to_lane(state.player.x_pos);
                bool player_in_center = (player_lane == LANE_CENTER);
                bool player_behind_center = (state.player.get_top() > center_car->get_bottom());

                if (player_in_center && player_behind_center)
                {
                    center_car->escape_target_speed = 3.5 * state.global_speed_multiplier * 0.0005;
                    center_car->speed = center_car->escape_target_speed;
                }
                else
                {
                    center_car->escape_target_speed = 3.5 * 1.5 * state.global_speed_multiplier;
                    center_car->speed = center_car->escape_target_speed;
                }
            }
        }

        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// renderer.cpp
// ---------------------------------------------------------------------------

void draw_road(GameState &state);
void draw_start_screen(GameState &state);
void draw_enemies(GameState &state);
void draw_player(PlayerCar &player);
void draw_hud(GameState &state);
void draw_game_over(GameState &state);

bitmap player_bitmap;
bitmap truck_bitmap;
bitmap minivan_bitmap;
bitmap sedan_bitmap;
bitmap reckless_bitmap;

double road_offset_y = 0.0;

void init_renderer()
{
    open_window("Three-Lane Highway", WINDOW_WIDTH, WINDOW_HEIGHT);

    player_bitmap = load_bitmap("player", "Resources/images/user_car.png");
    truck_bitmap = load_bitmap("truck", "Resources/images/truck.png");
    minivan_bitmap = load_bitmap("minivan", "Resources/images/minivan.png");
    sedan_bitmap = load_bitmap("sedan", "Resources/images/sedan.png");
}

void cleanup_renderer()
{
    free_bitmap(player_bitmap);
    free_bitmap(truck_bitmap);
    free_bitmap(minivan_bitmap);
    free_bitmap(sedan_bitmap);
}

void draw_game(GameState &state)
{
    clear_screen(COLOR_BLACK);

    if (state.phase == START_SCREEN)
    {
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

    refresh_screen(100);
}

void draw_road(GameState &state)
{
    double road_w = ROAD_WIDTH;
    double road_left = (WINDOW_WIDTH - road_w) / 2.0;
    double road_right = road_left + road_w;

    fill_rectangle(COLOR_GREEN, 0, 0, road_left, WINDOW_HEIGHT);
    fill_rectangle(COLOR_GREEN, road_right, 0, WINDOW_WIDTH - road_right, WINDOW_HEIGHT);

    fill_rectangle(COLOR_DIM_GRAY, road_left, 0, road_w, WINDOW_HEIGHT);

    if (state.phase == PLAYING)
    {
        road_offset_y += 5.0 * state.global_speed_multiplier;
        if (road_offset_y > 40.0)
            road_offset_y -= 40.0;
    }

    double lane_w = road_w / 3.0;
    for (int y = -40; y < WINDOW_HEIGHT; y += 40)
    {
        fill_rectangle(COLOR_WHITE, road_left + lane_w - 2, y + road_offset_y, 4, 20);
        fill_rectangle(COLOR_WHITE, road_left + 2 * lane_w - 2, y + road_offset_y, 4, 20);
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
        EnemyCar &e_car = state.active_enemies.get(i);
        bitmap vehicle;

        switch (e_car.type)
        {
        case TRUCK:
            vehicle = truck_bitmap;
            break;
        case MINIVAN:
            vehicle = minivan_bitmap;
            break;
        case SEDAN:
            vehicle = sedan_bitmap;
            break;
        case RECKLESS_CAR:
            vehicle = reckless_bitmap;
            break;
        default:
            vehicle = sedan_bitmap;
            break;
        }

        double draw_x = e_car.x_pos - bitmap_width(vehicle) / 2.0;
        double draw_y = e_car.y_pos;
        draw_bitmap(vehicle, draw_x, draw_y);
    }
}

void draw_start_screen(GameState &state)
{
    fill_rectangle(rgba_color(64, 64, 64, 150), 100, 100, 500, 500);
    draw_text("THREE-LANE ARCADE HIGHWAY RACER", COLOR_WHITE, 150, 200);
    draw_text("High Score: " + to_string(state.high_score), COLOR_YELLOW, 320, 260);
    draw_text("Press SPACE to Start", COLOR_WHITE, 300, 320);
    draw_text("Controls: LEFT / RIGHT Arrows", COLOR_WHITE, 280, 360);
}

void draw_hud(GameState &state)
{
    fill_rectangle(rgba_color(0, 0, 0, 150), 10, 10, 150, 60);
    draw_text("Score: " + to_string(state.score), COLOR_WHITE, 20, 20);
    draw_text("High: " + to_string(state.high_score), COLOR_YELLOW, 20, 40);
}

void draw_game_over(GameState &state)
{
    fill_rectangle(rgba_color(0, 0, 0, 200), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    draw_text("CRASHED!", COLOR_DARK_RED, 250, 200);
    draw_text("Final Score: " + to_string(state.score), COLOR_WHITE, 300, 280);
    draw_text("Press SPACE to Restart", COLOR_WHITE, 280, 330);
}

// ---------------------------------------------------------------------------
// input_handler.cpp
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// main.cpp
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// test_program.cpp
// ---------------------------------------------------------------------------

#ifdef INCLUDE_TEST_PROGRAM

using namespace std;

int tests_run = 0;
int tests_passed = 0;

#define TEST_ASSERT(condition, message)        \
    tests_run++;                               \
    if (condition)                             \
    {                                          \
        tests_passed++;                        \
        cout << "✓ PASS: " << message << endl; \
    }                                          \
    else                                       \
    {                                          \
        cout << "✗ FAIL: " << message << endl; \
    }

void test_game_types()
{
    cout << "\n=== Testing Game Types and Constants ===" << endl;
    TEST_ASSERT(WINDOW_WIDTH == 1000, "Window width is 1000");
    TEST_ASSERT(WINDOW_HEIGHT == 1000, "Window height is 1000");
    TEST_ASSERT(LANE_WIDTH == (ROAD_RIGHT - ROAD_LEFT) / 3, "Lane width is correct");
    TEST_ASSERT(PLAYER_CAR_WIDTH == 124, "Player car width is 124");
    TEST_ASSERT(PLAYER_CAR_HEIGHT == 126, "Player car height is 126");
    TEST_ASSERT(MAX_ENEMIES == 50, "Max enemies is 50");
}

void test_player_car()
{
    cout << "\n=== Testing Player Car ===" << endl;

    PlayerCar player;

    TEST_ASSERT(player.current_lane == LANE_CENTER, "Player starts in center lane");
    TEST_ASSERT(player.x_pos == lane_to_x(LANE_CENTER), "Player X position matches center lane");
    TEST_ASSERT(player.y_pos == PLAYER_Y, "Player Y position is correct");
    TEST_ASSERT(player.is_crashed == false, "Player is not crashed at start");

    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.target_x == lane_to_x(LANE_LEFT), "Target X changes on lane switch");

    double old_x = player.x_pos;
    player.update_position();
    TEST_ASSERT(player.x_pos != old_x || old_x == lane_to_x(LANE_LEFT),
                "Player position updates");

    TEST_ASSERT(player.get_width() == PLAYER_CAR_WIDTH, "Player width getter works");
    TEST_ASSERT(player.get_height() == PLAYER_CAR_HEIGHT, "Player height getter works");
    TEST_ASSERT(player.get_left() < player.get_right(), "Left < right");
    TEST_ASSERT(player.get_top() < player.get_bottom(), "Top < bottom");
}

void test_enemy_car()
{
    cout << "\n=== Testing Enemy Car ===" << endl;

    EnemyCar enemy;

    TEST_ASSERT(enemy.type == SEDAN, "Default enemy is SEDAN");
    TEST_ASSERT(enemy.behavior_type == CRUISING, "Default behavior is CRUISING");
    TEST_ASSERT(enemy.passed_player == false, "Enemy hasn't passed player");
    TEST_ASSERT(enemy.frames_alive == 0, "Enemy frames alive is 0");

    TEST_ASSERT(enemy.get_width() > 0, "Enemy width is positive");
    TEST_ASSERT(enemy.get_height() > 0, "Enemy height is positive");

    double initial_y = enemy.y_pos;
    enemy.speed = 2.0;
    enemy.update_position(1.0);
    TEST_ASSERT(enemy.y_pos > initial_y, "Enemy Y position increases");

    enemy.y_pos = WINDOW_HEIGHT + 200;
    TEST_ASSERT(enemy.is_off_screen() == true, "Enemy off-screen detection works");
}

void test_bounded_array()
{
    cout << "\n=== Testing Bounded Array ===" << endl;

    dynamic_array<int> arr;

    TEST_ASSERT(arr.length() == 0, "Array starts empty");
    TEST_ASSERT(arr.capacity() == 10, "Array capacity is correct");

    arr.add(5);
    TEST_ASSERT(arr.length() == 1, "Array length increases after add");
    TEST_ASSERT(arr.get(0) == 5, "Can retrieve added element");

    arr.add(10);
    arr.add(15);
    TEST_ASSERT(arr.length() == 3, "Array has 3 elements");
    TEST_ASSERT(arr.get(1) == 10, "Second element is correct");
    TEST_ASSERT(arr.get(2) == 15, "Third element is correct");

    arr.remove_at(1);
    TEST_ASSERT(arr.length() == 2, "Array length decreases after remove");
    TEST_ASSERT(arr.get(1) == 15, "Elements shift correctly after remove");
}

void test_lane_to_x()
{
    cout << "\n=== Testing Lane to X Conversion ===" << endl;

    double left_x = lane_to_x(LANE_LEFT);
    double center_x = lane_to_x(LANE_CENTER);
    double right_x = lane_to_x(LANE_RIGHT);

    TEST_ASSERT(left_x > 0, "Left lane X is positive");
    TEST_ASSERT(center_x > 0, "Center lane X is positive");
    TEST_ASSERT(right_x > 0, "Right lane X is positive");

    TEST_ASSERT(left_x < center_x && center_x < right_x, "Lanes ordered left to right");
    TEST_ASSERT(right_x - left_x == 2 * LANE_WIDTH / 3.0, "Lane spacing is correct");
}

void test_game_state()
{
    cout << "\n=== Testing Game State ===" << endl;

    GameState state;
    initialize_game(state);

    TEST_ASSERT(state.phase == START_SCREEN, "Game starts at START_SCREEN");
    TEST_ASSERT(state.score == 0, "Score starts at 0");
    TEST_ASSERT(state.global_speed_multiplier == 1.0, "Speed multiplier starts at 1.0");
    TEST_ASSERT(state.active_enemies.length() == 0, "No enemies at start");

    reset_gameplay(state);
    TEST_ASSERT(state.phase == PLAYING, "Phase changes to PLAYING on reset");
    TEST_ASSERT(state.score == 0, "Score resets to 0");
    TEST_ASSERT(state.active_enemies.length() == 0, "Enemies cleared on reset");
}

void test_can_spawn_in_lane()
{
    cout << "\n=== Testing can_spawn_in_lane Function ===" << endl;

    GameState state;
    initialize_game(state);

    double out_speed = 0.0;
    int result = can_spawn_in_lane(state, LANE_LEFT, &out_speed);
    TEST_ASSERT(result == 1, "Empty lane returns 1");

    EnemyCar car;
    car.current_lane = LANE_CENTER;
    car.speed = 2.5;
    car.y_pos = 100.0;
    state.active_enemies.add(car);

    out_speed = 0.0;
    result = can_spawn_in_lane(state, LANE_CENTER, &out_speed);
    TEST_ASSERT(result == 2, "Single car returns 2 (follow speed)");
    TEST_ASSERT(out_speed == 2.5, "Out speed matches the car's speed");

    EnemyCar car2;
    car2.current_lane = LANE_CENTER;
    car2.speed = 3.0;
    car2.y_pos = 200.0;
    state.active_enemies.add(car2);

    result = can_spawn_in_lane(state, LANE_CENTER, &out_speed);
    TEST_ASSERT(result == 0, "Two cars in lane returns 0 (cannot spawn)");
}

void test_can_change_lane()
{
    cout << "\n=== Testing can_change_lane Function ===" << endl;

    GameState state;
    initialize_game(state);

    bool result = can_change_lane(state, lane_to_x(LANE_CENTER), 300.0,
                                  SEDAN_WIDTH, SEDAN_HEIGHT, LANE_LEFT);
    TEST_ASSERT(result == true, "Can change lane to empty lane");

    EnemyCar blocker;
    blocker.type = SEDAN;
    blocker.current_lane = LANE_LEFT;
    blocker.x_pos = lane_to_x(LANE_LEFT);
    blocker.y_pos = 290.0;
    blocker.speed = 2.0;
    state.active_enemies.add(blocker);

    result = can_change_lane(state, lane_to_x(LANE_CENTER), 300.0,
                             SEDAN_WIDTH, SEDAN_HEIGHT, LANE_LEFT);
    TEST_ASSERT(result == false, "Cannot change lane if car blocking");
}

void test_prevent_3lane_blockade()
{
    cout << "\n=== Testing prevent_3lane_blockade Function ===" << endl;

    GameState state;
    initialize_game(state);

    bool result = prevent_3lane_blockade(state, LANE_CENTER, SEDAN_WIDTH, SEDAN_HEIGHT);
    TEST_ASSERT(result == true, "Empty lanes allow spawning");

    EnemyCar left_car;
    left_car.type = SEDAN;
    left_car.current_lane = LANE_LEFT;
    left_car.x_pos = lane_to_x(LANE_LEFT);
    left_car.y_pos = 300.0;
    state.active_enemies.add(left_car);

    EnemyCar right_car;
    right_car.type = SEDAN;
    right_car.current_lane = LANE_RIGHT;
    right_car.x_pos = lane_to_x(LANE_RIGHT);
    right_car.y_pos = 310.0;
    state.active_enemies.add(right_car);

    result = prevent_3lane_blockade(state, LANE_CENTER, SEDAN_WIDTH, SEDAN_HEIGHT);
    TEST_ASSERT(result == false, "Blockade prevention triggers");

    result = prevent_3lane_blockade(state, LANE_LEFT, SEDAN_WIDTH, SEDAN_HEIGHT);
    TEST_ASSERT(result == true, "Can spawn in original lane");
}

void test_speed_multiplier()
{
    cout << "\n=== Testing Speed Multiplier Logic ===" << endl;

    GameState state;
    initialize_game(state);
    reset_gameplay(state);

    state.score = 500;
    update_game(state);
    TEST_ASSERT(state.global_speed_multiplier == 1.0, "Multiplier 1.0 below level 2");

    state.score = LEVEL_2_SCORE + 100;
    update_game(state);
    TEST_ASSERT(state.global_speed_multiplier == 1.5, "Multiplier 1.5 at level 2");

    state.score = LEVEL_3_SCORE + 100;
    update_game(state);
    TEST_ASSERT(state.global_speed_multiplier == 2.0, "Multiplier 2.0 at level 3");
}

void test_random_numbers()
{
    cout << "\n=== Testing Random Number Generation ===" << endl;

    int min_val = 10, max_val = 20;
    bool found_min = false, found_max = false;

    for (int i = 0; i < 100; i++)
    {
        int rand_val = random_int(min_val, max_val);
        TEST_ASSERT(rand_val >= min_val && rand_val <= max_val,
                    "Random int in range");
        if (rand_val == min_val)
            found_min = true;
        if (rand_val == max_val)
            found_max = true;
    }

    TEST_ASSERT(found_min, "Found minimum value");
    TEST_ASSERT(found_max, "Found maximum value");

    double rand_double = random_double(0.0, 1.0);
    TEST_ASSERT(rand_double >= 0.0 && rand_double <= 1.0, "Random double in range");
}

void test_vehicle_dimensions()
{
    cout << "\n=== Testing Vehicle Dimensions ===" << endl;

    EnemyCar truck;
    truck.type = TRUCK;
    TEST_ASSERT(truck.get_width() == TRUCK_WIDTH, "Truck width correct");
    TEST_ASSERT(truck.get_height() == TRUCK_HEIGHT, "Truck height correct");

    EnemyCar sedan;
    sedan.type = SEDAN;
    TEST_ASSERT(sedan.get_width() == SEDAN_WIDTH, "Sedan width correct");
    TEST_ASSERT(sedan.get_height() == SEDAN_HEIGHT, "Sedan height correct");
}

void test_x_to_lane()
{
    cout << "\n=== Testing x_to_lane Function ===" << endl;

    double left_x = lane_to_x(LANE_LEFT);
    double center_x = lane_to_x(LANE_CENTER);
    double right_x = lane_to_x(LANE_RIGHT);

    Lane result_left = x_to_lane(left_x);
    Lane result_center = x_to_lane(center_x);
    Lane result_right = x_to_lane(right_x);

    TEST_ASSERT(result_left == LANE_LEFT, "x_to_lane correctly identifies left lane");
    TEST_ASSERT(result_center == LANE_CENTER, "x_to_lane correctly identifies center lane");
    TEST_ASSERT(result_right == LANE_RIGHT, "x_to_lane correctly identifies right lane");
}

void test_count_cars_in_lane()
{
    cout << "\n=== Testing count_cars_in_lane Function ===" << endl;

    GameState state;
    initialize_game(state);

    TEST_ASSERT(count_cars_in_lane(state, LANE_LEFT) == 0, "Empty lane has 0 cars");

    EnemyCar car1;
    car1.current_lane = LANE_LEFT;
    state.active_enemies.add(car1);

    TEST_ASSERT(count_cars_in_lane(state, LANE_LEFT) == 1, "Lane has 1 car");

    EnemyCar car2;
    car2.current_lane = LANE_LEFT;
    state.active_enemies.add(car2);

    TEST_ASSERT(count_cars_in_lane(state, LANE_LEFT) == 2, "Lane has 2 cars");
    TEST_ASSERT(count_cars_in_lane(state, LANE_CENTER) == 0, "Other lane unaffected");
}

void test_enemy_center_y()
{
    cout << "\n=== Testing enemy_center_y Function ===" << endl;

    EnemyCar enemy;
    enemy.y_pos = 100.0;
    double height = static_cast<double>(enemy.get_height());

    double center_y = enemy_center_y(enemy);
    TEST_ASSERT(center_y == 100.0 + height / 2.0, "Enemy center Y calculated correctly");
}

void test_lanes_are_adjacent()
{
    cout << "\n=== Testing lanes_are_adjacent Function ===" << endl;

    TEST_ASSERT(lanes_are_adjacent(LANE_LEFT, LANE_CENTER), "Left and center are adjacent");
    TEST_ASSERT(lanes_are_adjacent(LANE_CENTER, LANE_LEFT), "Center and left are adjacent (symmetric)");
    TEST_ASSERT(lanes_are_adjacent(LANE_CENTER, LANE_RIGHT), "Center and right are adjacent");
    TEST_ASSERT(lanes_are_adjacent(LANE_RIGHT, LANE_CENTER), "Right and center are adjacent (symmetric)");
    TEST_ASSERT(!lanes_are_adjacent(LANE_LEFT, LANE_RIGHT), "Left and right are not adjacent");
    TEST_ASSERT(!lanes_are_adjacent(LANE_RIGHT, LANE_LEFT), "Right and left are not adjacent");
}

void test_player_crash_state()
{
    cout << "\n=== Testing Player Car Crash State ===" << endl;

    PlayerCar player;
    TEST_ASSERT(!player.is_crashed, "Player starts not crashed");

    player.is_crashed = true;
    TEST_ASSERT(player.is_crashed, "Player can be marked as crashed");

    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.current_lane == LANE_CENTER, "Crashed player cannot switch lanes");
}

void test_array_capacity_and_resize()
{
    cout << "\n=== Testing Dynamic Array Capacity and Resize ===" << endl;

    dynamic_array<int> arr(5);
    TEST_ASSERT(arr.capacity() == 5, "Initial capacity is 5");

    for (int i = 0; i < 10; i++)
    {
        arr.add(i * 10);
    }

    TEST_ASSERT(arr.length() == 10, "Array grew to 10 elements");
    TEST_ASSERT(arr.capacity() >= 10, "Array capacity expanded");

    bool all_correct = true;
    for (int i = 0; i < 10; i++)
    {
        if (arr.get(i) != i * 10)
            all_correct = false;
    }
    TEST_ASSERT(all_correct, "All elements preserved after resize");
}

void test_enemy_off_screen_states()
{
    cout << "\n=== Testing Enemy Off-Screen Detection ===" << endl;

    EnemyCar enemy;
    enemy.y_pos = -50;
    TEST_ASSERT(!enemy.is_off_screen(), "Enemy above screen is on-screen");

    enemy.y_pos = 500;
    TEST_ASSERT(!enemy.is_off_screen(), "Enemy in middle is on-screen");

    enemy.y_pos = WINDOW_HEIGHT + 200;
    TEST_ASSERT(enemy.is_off_screen(), "Enemy below screen is off-screen");
}

void test_player_lane_switch_mechanics()
{
    cout << "\n=== Testing Player Lane Switch Mechanics ===" << endl;

    PlayerCar player;
    player.current_lane = LANE_CENTER;

    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.current_lane == LANE_LEFT, "Current lane updated");
    TEST_ASSERT(player.target_lane == LANE_LEFT, "Target lane updated");

    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.current_lane == LANE_LEFT, "Cannot switch to same lane");

    player.switch_lane(LANE_RIGHT);
    TEST_ASSERT(player.current_lane == LANE_RIGHT, "Can switch from left to center to right");
}

int main()
{
    cout << "\n"
         << string(60, '=') << endl;
    cout << "  THREE-LANE HIGHWAY GAME - COMPREHENSIVE TEST SUITE" << endl;
    cout << string(60, '=') << endl;

    test_game_types();
    test_player_car();
    test_enemy_car();
    test_bounded_array();
    test_lane_to_x();
    test_game_state();
    test_can_spawn_in_lane();
    test_can_change_lane();
    test_prevent_3lane_blockade();
    test_speed_multiplier();
    test_random_numbers();
    test_vehicle_dimensions();
    test_x_to_lane();
    test_count_cars_in_lane();
    test_enemy_center_y();
    test_lanes_are_adjacent();
    test_player_crash_state();
    test_array_capacity_and_resize();
    test_enemy_off_screen_states();
    test_player_lane_switch_mechanics();

    cout << "\n"
         << string(60, '=') << endl;
    cout << "TEST RESULTS SUMMARY" << endl;
    cout << string(60, '=') << endl;
    cout << "Tests Run:    " << tests_run << endl;
    cout << "Tests Passed: " << tests_passed << endl;
    cout << "Tests Failed: " << (tests_run - tests_passed) << endl;

    if (tests_run > 0)
    {
        double pass_rate = (static_cast<double>(tests_passed) / tests_run) * 100.0;
        cout << "Pass Rate:    " << pass_rate << "%" << endl;
    }

    cout << string(60, '=') << endl;

    if (tests_passed == tests_run)
    {
        cout << "\n✓ ALL TESTS PASSED!" << endl;
        return 0;
    }
    else
    {
        cout << "\n✗ SOME TESTS FAILED" << endl;
        return 1;
    }
}

#endif // INCLUDE_TEST_PROGRAM
