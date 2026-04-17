#pragma once
#include <string>

struct Action {
    double time;
    std::string type; // "down" or "up"
    char key;
};