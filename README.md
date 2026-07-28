# Three-Lane Highway Racer

A fast-paced arcade highway racing game built in C++ using the SplashKit framework. Dodge oncoming traffic across three lanes, survive as long as possible, and beat your high score.

---

## Gameplay

You control a car at the bottom of the screen. Enemy vehicles spawn at the top and drive toward you. Switch lanes to avoid them — every car you survive past adds to your score. The game gets faster as your score climbs.

- **Left / Right arrow keys** — switch lanes
- **Space** — start or restart the game

---

## Features

- Three difficulty levels that scale with your score, increasing the global speed multiplier
- Three enemy vehicle types: trucks (slow), minivans (medium), and sedans (fast)
- Anti-blockade system that detects when all three lanes are simultaneously occupied side-by-side and resolves the situation so the player always has a path through
- Spawn logic that prevents unsafe lane configurations, including cars spawning too close together or in patterns that would immediately block the player
- Slipstream effect that slows center-lane cars when they are flanked on both sides, simulating a realistic traffic squeeze
- High score persistence saved to a local JSON file between sessions
- Smooth lane-switching interpolation for the player car

---

## Project Structure

```
consolidated.cpp        — All game source code in one file
Resources/
  images/
    user_car.png        — Player vehicle sprite
    truck.png           — Truck sprite
    minivan.png         — Minivan sprite
    sedan.png           — Sedan sprite
highscore.json          — Auto-generated on first run
```

The source is organised into logical sections within the single file, each corresponding to a conceptual module:

| Section | Responsibility |
|---|---|
| `dynamic_array.hpp` | Generic resizable array template (custom, no STL) |
| `game_types.cpp` | Constants, enums, and the `lane_to_x()` helper |
| `player.cpp` | `PlayerCar` class — movement, lane switching, collision bounds |
| `enemy.cpp` | `EnemyCar` class — movement, off-screen detection, vehicle dimensions |
| `game_state.cpp` | Core game logic — spawning, collision, blockade resolution, scoring |
| `score_io.cpp` | High score read/write using SplashKit's JSON API |
| `renderer.cpp` | All drawing — road, HUD, start screen, game over overlay |
| `input_handler.cpp` | Keyboard input routed by game phase |
| `main.cpp` | Game loop entry point |

---

## Difficulty Scaling

| Level | Score threshold | Speed multiplier |
|---|---|---|
| Level 1 | 0 – 1199 | 1.0× |
| Level 2 | 1200 – 2999 | 1.5× |
| Level 3 | 3000+ | 2.0× |

---

## Building

This project requires [SplashKit](https://splashkit.io) to be installed.

```bash
skm clang++ consolidated.cpp -o highway_game
./highway_game
```

To build and run the test suite:

```bash
skm clang++ -D INCLUDE_TEST_PROGRAM consolidated.cpp -o highway_tests
./highway_tests
```

---

## Running Tests

The test suite covers:

- Game constants and types
- Player car initialisation, lane switching, and crash state
- Enemy car movement, off-screen detection, and vehicle dimensions
- `dynamic_array` — add, get, remove, resize, and capacity
- Lane conversion functions (`lane_to_x`, `x_to_lane`)
- Game state initialisation and reset
- Spawn logic (`can_spawn_in_lane`, `prevent_3lane_blockade`)
- Speed multiplier thresholds
- Helper functions (`count_cars_in_lane`, `lanes_are_adjacent`, `enemy_center_y`)

---

## Known Limitations

- `BehaviorType` (`CRUISING`, `OVERTAKING`, `RECKLESS`) is assigned to enemies during spawning but is not currently used to drive any behaviour — this was planned but not fully implemented
- `RECKLESS_CAR` vehicle type exists in the codebase but is never spawned
- The forced escape fallback in `resolve_impasse()` hardcodes `LANE_LEFT` and does not check `can_change_lane()` before moving, which can in rare cases cause an unsafe lane change
- Game speed is frame-rate dependent — movement is calculated in fixed pixels per frame rather than using delta-time

---

## Dependencies

- [SplashKit](https://splashkit.io) — graphics, input, audio, and JSON handling
- C++11 or later

---

## Academic Context

This game was built as the final assessment for **FIT1045 — Introduction to Programming** at Monash University. It achieved a raw mark of **90% (High Distinction)**.

---

## License

This project was created as a university assessment submission.
