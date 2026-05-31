#include "game_types.cpp"
#include <cmath>

class PlayerCar
{
    public:
        PlayerCar()
        {
            current_lane = LANE_CENTER;
            target_lane  = LANE_CENTER;
            x_pos        = lane_to_x(LANE_CENTER);
            target_x     = x_pos;
            y_pos        = PLAYER_Y;
            is_crashed   = false;
        }
        void switch_lane(Lane new_lane);
        void update_position();

        int get_width();
        int get_height();
        double get_left();
        double get_right();
        double get_top();
        double get_bottom();

        // Public fields (kept simple)
        Lane current_lane;
        Lane target_lane;
        double x_pos;
        double target_x;
        double y_pos;
        bool is_crashed;
};



void PlayerCar::switch_lane(Lane new_lane)
{
    if (is_crashed) return;

    if (new_lane == LANE_LEFT  && current_lane == LANE_LEFT)  return;
    if (new_lane == LANE_RIGHT && current_lane == LANE_RIGHT) return;

    target_lane = new_lane;
    target_x    = lane_to_x(new_lane);
    current_lane = new_lane; 
}

void PlayerCar::update_position()
{
    double diff = target_x - x_pos;

    if (std::fabs(diff) <= LANE_LERP_SPEED)
        x_pos = target_x;   
    else
        x_pos += (diff > 0 ? LANE_LERP_SPEED : -LANE_LERP_SPEED);
}

int PlayerCar::get_width()  { return PLAYER_CAR_WIDTH;  }
int PlayerCar::get_height() { return PLAYER_CAR_HEIGHT; }
double PlayerCar::get_left()   { return x_pos - PLAYER_CAR_WIDTH  / 2.0; }
double PlayerCar::get_right()  { return x_pos + PLAYER_CAR_WIDTH  / 2.0; }
double PlayerCar::get_top()    { return y_pos; }
double PlayerCar::get_bottom() { return y_pos + PLAYER_CAR_HEIGHT; }
