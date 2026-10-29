#pragma once

#include "CityGraph.h"
#include "Route.h"

#include <cstddef>
#include <string>
#include <vector>

// Returns up to k distinct routes, ordered by total distance.
std::vector<Route> kShortestPaths(
    const CityGraph &graph,
    const std::string &src,
    const std::string &dst,
    std::size_t k = 3
    );