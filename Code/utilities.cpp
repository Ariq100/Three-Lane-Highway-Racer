#include "utilities.h"
#include "splashkit.h" 

string read_string(string prompt)
{
    write(prompt);
    return read_line();
}

int read_integer(string prompt)
{
    string input = read_string(prompt);

    while (!is_integer(input))
    {
        write("Please enter a whole number: ");

        input = read_string(prompt);
    }

    return to_integer(input);
}

double read_double(string prompt) {
    string input = read_string(prompt);

    while (!is_number(input)) {
        write("Please enter a number: ");
        
        input = read_string(prompt);
    }

    return to_double(input);
}

bool read_boolean(string prompt) {
    while (true) {
        string input = read_string(prompt);
        input = trim(to_lowercase(input));
        if (input == "y" || input == "yes" || input == "1") {
            return true;
        } else if (input == "n" || input == "no" || input == "0") {
            return false;
        } else {
            write_line("Please enter yes or no.");
        }
    }
}