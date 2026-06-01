#include "splashkit.h"
#include <string>
#include "rng.h"

int load_highscore(const std::string &filepath)
{
    int highscore = 0;

    // Try to load the JSON file
    json j = json_from_file(filepath);

    // If successful and it contains "highscore"
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
