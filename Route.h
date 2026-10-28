#pragma once
#include <string>
#include <vector>

struct Route {
    std::vector<std::string> path;   // ordered location names
    double totalDistance = 0.0;
    bool valid = false;              // false if unreachable or invalid input
};
