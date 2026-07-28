#include "game_types.cpp"
#include "rng.h"
#include <cmath>
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

    // Default constructor
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
    // Advance position each frame
    void update_position(double global_multiplier)
    {
        frames_alive++;
        if (lane_change_timer > 0)
            lane_change_timer--;

        // Move downward each frame at scaled speed
        double travel = speed * global_multiplier;
        y_pos += travel;

        if (awaiting_overtake)
        {
            overtake_progress += std::fabs(travel);
        }

        // Reckless behaviour is intentionally disabled for now (spawn disabled too)

        // Lerp toward target_x
        double diff = target_x - x_pos;
        if (std::fabs(diff) > 2.0)
            x_pos += diff * 0.08; // Smooth swerving
        else
            x_pos = target_x;
    }

    // True when the car has scrolled fully below the bottom of the screen
    bool is_off_screen()
    {
        return y_pos > static_cast<double>(WINDOW_HEIGHT + get_height());
    }


    // Hitbox helpers
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