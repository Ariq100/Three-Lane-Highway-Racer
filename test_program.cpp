// ---------------------------------------------------------------------------
// test_program.cpp
// Comprehensive test program for the three-lane highway game
//
// COMPILATION COMMAND:
// clang++ -std=c++17 -I. test_program.cpp game_state.cpp enemy.cpp player.cpp \
//         renderer.cpp input_handler.cpp score_io.cpp rng.cpp \
//         -o test_runner -lsplashkit
//
// ---------------------------------------------------------------------------
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include "splashkit.h"
#include "game_types.cpp"
#include "game_state.cpp"
#include "rng.h"
#include "dynamic_array.hpp"

using namespace std;

// Test counter
int tests_run = 0;
int tests_passed = 0;

// Helper macro for assertions
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

// ---------------------------------------------------------------------------
// Test 1: Game Types and Constants
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 2: Player Car Functionality
// ---------------------------------------------------------------------------
void test_player_car()
{
    cout << "\n=== Testing Player Car ===" << endl;

    PlayerCar player;

    TEST_ASSERT(player.current_lane == LANE_CENTER, "Player starts in center lane");
    TEST_ASSERT(player.x_pos == lane_to_x(LANE_CENTER), "Player X position matches center lane");
    TEST_ASSERT(player.y_pos == PLAYER_Y, "Player Y position is correct");
    TEST_ASSERT(player.is_crashed == false, "Player is not crashed at start");

    // Test lane switching
    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.target_x == lane_to_x(LANE_LEFT), "Target X changes on lane switch");

    // Test update position (should move towards target)
    double old_x = player.x_pos;
    player.update_position();
    TEST_ASSERT(player.x_pos != old_x || old_x == lane_to_x(LANE_LEFT),
                "Player position updates");

    // Test hitbox calculations
    TEST_ASSERT(player.get_width() == PLAYER_CAR_WIDTH, "Player width getter works");
    TEST_ASSERT(player.get_height() == PLAYER_CAR_HEIGHT, "Player height getter works");
    TEST_ASSERT(player.get_left() < player.get_right(), "Left < right");
    TEST_ASSERT(player.get_top() < player.get_bottom(), "Top < bottom");
}

// ---------------------------------------------------------------------------
// Test 3: Enemy Car Functionality
// ---------------------------------------------------------------------------
void test_enemy_car()
{
    cout << "\n=== Testing Enemy Car ===" << endl;

    EnemyCar enemy;

    TEST_ASSERT(enemy.type == SEDAN, "Default enemy is SEDAN");
    TEST_ASSERT(enemy.behavior_type == CRUISING, "Default behavior is CRUISING");
    TEST_ASSERT(enemy.passed_player == false, "Enemy hasn't passed player");
    TEST_ASSERT(enemy.frames_alive == 0, "Enemy frames alive is 0");

    // Test hitbox getters
    TEST_ASSERT(enemy.get_width() > 0, "Enemy width is positive");
    TEST_ASSERT(enemy.get_height() > 0, "Enemy height is positive");

    // Test position update
    double initial_y = enemy.y_pos;
    enemy.speed = 2.0;
    enemy.update_position(1.0);
    TEST_ASSERT(enemy.y_pos > initial_y, "Enemy Y position increases");

    // Test off-screen detection
    enemy.y_pos = WINDOW_HEIGHT + 200;
    TEST_ASSERT(enemy.is_off_screen() == true, "Enemy off-screen detection works");
}

// ---------------------------------------------------------------------------
// Test 4: Bounded Array
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 5: Lane-to-X Coordinate Conversion
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 6: Game State and Initialization
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 7: Traffic Management - can_spawn_in_lane
// ---------------------------------------------------------------------------
void test_can_spawn_in_lane()
{
    cout << "\n=== Testing can_spawn_in_lane Function ===" << endl;

    GameState state;
    initialize_game(state);

    // Test 1: Empty lane should allow spawning
    double out_speed = 0.0;
    int result = can_spawn_in_lane(state, LANE_LEFT, &out_speed);
    TEST_ASSERT(result == 1, "Empty lane returns 1");

    // Test 2: Add a car to the lane
    EnemyCar car;
    car.current_lane = LANE_CENTER;
    car.speed = 2.5;
    car.y_pos = 100.0;
    state.active_enemies.add(car);

    out_speed = 0.0;
    result = can_spawn_in_lane(state, LANE_CENTER, &out_speed);
    TEST_ASSERT(result == 2, "Single car returns 2 (follow speed)");
    TEST_ASSERT(out_speed == 2.5, "Out speed matches the car's speed");

    // Test 3: Add another car to the same lane
    EnemyCar car2;
    car2.current_lane = LANE_CENTER;
    car2.speed = 3.0;
    car2.y_pos = 200.0;
    state.active_enemies.add(car2);

    result = can_spawn_in_lane(state, LANE_CENTER, &out_speed);
    TEST_ASSERT(result == 0, "Two cars in lane returns 0 (cannot spawn)");
}

// ---------------------------------------------------------------------------
// Test 8: Lane Changing Validation
// ---------------------------------------------------------------------------
void test_can_change_lane()
{
    cout << "\n=== Testing can_change_lane Function ===" << endl;

    GameState state;
    initialize_game(state);

    // Empty lanes - should be able to change
    bool result = can_change_lane(state, lane_to_x(LANE_CENTER), 300.0,
                                  SEDAN_WIDTH, SEDAN_HEIGHT, LANE_LEFT);
    TEST_ASSERT(result == true, "Can change lane to empty lane");

    // Add a car in the target lane that would cause collision
    EnemyCar blocker;
    blocker.type = SEDAN;
    blocker.current_lane = LANE_LEFT;
    blocker.x_pos = lane_to_x(LANE_LEFT);
    blocker.y_pos = 290.0; // Very close to our car
    blocker.speed = 2.0;
    state.active_enemies.add(blocker);

    result = can_change_lane(state, lane_to_x(LANE_CENTER), 300.0,
                             SEDAN_WIDTH, SEDAN_HEIGHT, LANE_LEFT);
    TEST_ASSERT(result == false, "Cannot change lane if car blocking");
}

// ---------------------------------------------------------------------------
// Test 9: 3-Lane Blockade Prevention
// ---------------------------------------------------------------------------
void test_prevent_3lane_blockade()
{
    cout << "\n=== Testing prevent_3lane_blockade Function ===" << endl;

    GameState state;
    initialize_game(state);

    // Empty - should allow
    bool result = prevent_3lane_blockade(state, LANE_CENTER, SEDAN_WIDTH, SEDAN_HEIGHT);
    TEST_ASSERT(result == true, "Empty lanes allow spawning");

    // Add cars to two lanes close together
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
    right_car.y_pos = 310.0; // Close to left car
    state.active_enemies.add(right_car);

    // Should prevent spawning in center (third lane)
    result = prevent_3lane_blockade(state, LANE_CENTER, SEDAN_WIDTH, SEDAN_HEIGHT);
    TEST_ASSERT(result == false, "Blockade prevention triggers");

    // Should allow in an empty original lane
    result = prevent_3lane_blockade(state, LANE_LEFT, SEDAN_WIDTH, SEDAN_HEIGHT);
    TEST_ASSERT(result == true, "Can spawn in original lane");
}

// ---------------------------------------------------------------------------
// Test 10: Speed Multiplier Logic
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 11: Random Number Generation
// ---------------------------------------------------------------------------
void test_random_numbers()
{
    cout << "\n=== Testing Random Number Generation ===" << endl;

    // Test integer RNG multiple times
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

    // Test double RNG
    double rand_double = random_double(0.0, 1.0);
    TEST_ASSERT(rand_double >= 0.0 && rand_double <= 1.0, "Random double in range");
}

// ---------------------------------------------------------------------------
// Test 12: Vehicle Type Dimensions
// ---------------------------------------------------------------------------
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

    EnemyCar reckless;
    reckless.type = RECKLESS_CAR;
    TEST_ASSERT(reckless.get_width() == RECKLESS_WIDTH, "Reckless width correct");
    TEST_ASSERT(reckless.get_height() == RECKLESS_HEIGHT, "Reckless height correct");
}

// ---------------------------------------------------------------------------
// Test 13: x_to_lane Function
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 14: count_cars_in_lane Function
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 15: Enemy Center Y Calculation
// ---------------------------------------------------------------------------
void test_enemy_center_y()
{
    cout << "\n=== Testing enemy_center_y Function ===" << endl;

    EnemyCar enemy;
    enemy.y_pos = 100.0;
    double height = static_cast<double>(enemy.get_height());

    double center_y = enemy_center_y(enemy);
    TEST_ASSERT(center_y == 100.0 + height / 2.0, "Enemy center Y calculated correctly");
}

// ---------------------------------------------------------------------------
// Test 16: Lane Adjacency
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 17: Player Car Crash State
// ---------------------------------------------------------------------------
void test_player_crash_state()
{
    cout << "\n=== Testing Player Car Crash State ===" << endl;

    PlayerCar player;
    TEST_ASSERT(!player.is_crashed, "Player starts not crashed");

    player.is_crashed = true;
    TEST_ASSERT(player.is_crashed, "Player can be marked as crashed");

    // Test that crashed player cannot change lanes
    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.current_lane == LANE_CENTER, "Crashed player cannot switch lanes");
}

// ---------------------------------------------------------------------------
// Test 18: Dynamic Array Capacity
// ---------------------------------------------------------------------------
void test_array_capacity_and_resize()
{
    cout << "\n=== Testing Dynamic Array Capacity and Resize ===" << endl;

    dynamic_array<int> arr(5);
    TEST_ASSERT(arr.capacity() == 5, "Initial capacity is 5");

    // Add elements beyond initial capacity
    for (int i = 0; i < 10; i++)
    {
        arr.add(i * 10);
    }

    TEST_ASSERT(arr.length() == 10, "Array grew to 10 elements");
    TEST_ASSERT(arr.capacity() >= 10, "Array capacity expanded");

    // Check all elements are still there
    bool all_correct = true;
    for (int i = 0; i < 10; i++)
    {
        if (arr.get(i) != i * 10)
            all_correct = false;
    }
    TEST_ASSERT(all_correct, "All elements preserved after resize");
}

// ---------------------------------------------------------------------------
// Test 19: Enemy Off-Screen States
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Test 20: Player Lane Switch Mechanics
// ---------------------------------------------------------------------------
void test_player_lane_switch_mechanics()
{
    cout << "\n=== Testing Player Lane Switch Mechanics ===" << endl;

    PlayerCar player;
    player.current_lane = LANE_CENTER;

    // Switch to left
    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.current_lane == LANE_LEFT, "Current lane updated");
    TEST_ASSERT(player.target_lane == LANE_LEFT, "Target lane updated");

    // Try to switch to same lane
    player.switch_lane(LANE_LEFT);
    TEST_ASSERT(player.current_lane == LANE_LEFT, "Cannot switch to same lane");

    // Switch to right
    player.switch_lane(LANE_RIGHT);
    TEST_ASSERT(player.current_lane == LANE_RIGHT, "Can switch from left to center to right");
}

// ---------------------------------------------------------------------------
// Main Test Runner
// ---------------------------------------------------------------------------
int main()
{
    cout << "\n"
         << string(60, '=') << endl;
    cout << "  THREE-LANE HIGHWAY GAME - COMPREHENSIVE TEST SUITE" << endl;
    cout << string(60, '=') << endl;

    // Run all tests
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

    // Print summary
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
