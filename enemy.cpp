// ---------------------------------------------------------------------------
// enemy.cpp
// Enemy car implementation.
// ---------------------------------------------------------------------------
#include "game_types.cpp"
#include "rng.h"
#include <cmath>

// Minimal EnemyCar declaration (headers were removed by user request)
class EnemyCar
{
public:
    EnemyCar();
    void update_position(double global_multiplier);
    bool is_off_screen();

    int get_width();
    int get_height();
    double get_left();
    double get_right();
    double get_top();
    double get_bottom();

    // Public fields
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
};


// Default constructor
EnemyCar::EnemyCar()
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
}

// Advance position each frame
void EnemyCar::update_position(double global_multiplier)
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
        x_pos += diff * 0.08;  // Smooth swerving
    else
        x_pos = target_x;
}

// True when the car has scrolled fully below the bottom of the screen
bool EnemyCar::is_off_screen()
{
    return y_pos > static_cast<double>(WINDOW_HEIGHT + get_height());
}

// Hitbox helpers
int EnemyCar::get_width()
{
    switch (type)
    {
        case TRUCK:    
           return TRUCK_WIDTH;
        case MINIVAN:     
            return MINIVAN_WIDTH;
        case RECKLESS_CAR:
            return RECKLESS_WIDTH;
        default:          
            return SEDAN_WIDTH;
    }
}

int EnemyCar::get_height()
{
    switch (type)
    {
        case TRUCK:    
           return TRUCK_HEIGHT;
        case MINIVAN:  
           return MINIVAN_HEIGHT;
        case RECKLESS_CAR:
            return RECKLESS_HEIGHT;
        default:         
            return SEDAN_HEIGHT;
    }
}

double EnemyCar::get_left()
{
    return x_pos - get_width() / 2.0; 
}

double EnemyCar::get_right()
{
    return x_pos + get_width() / 2.0; 
}

double EnemyCar::get_top()
{
    return y_pos; 
}

double EnemyCar::get_bottom()
{
    return y_pos + get_height(); 
}