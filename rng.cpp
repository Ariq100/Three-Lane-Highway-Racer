#include "rng.h"


int random_int(int min, int max)
{
    return min + rnd(max - min + 1);
}

double random_double(double min, double max)
{
    return min + rnd() * (max - min);
}
