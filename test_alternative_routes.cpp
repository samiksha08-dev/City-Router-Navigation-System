#include "AlternativeRoutes.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include <vector>

int main()
{
    CityGraph graph;

    // Test 1: Find three routes between two locations.
    auto routes = kShortestPaths(
        graph, "Pune Station", "Hadapsar", 3
        );

    assert(routes.size() == 3);

    std::set<std::vector<std::string>> uniquePaths;

    for (const Route &route : routes)
    {
        // Every returned route must be valid.
        assert(route.valid);
        assert(!route.path.empty());

        // Verify the source and destination.
        assert(route.path.front() == "Pune Station");
        assert(route.path.back() == "Hadapsar");

        // No duplicate routes.
        assert(uniquePaths.insert(route.path).second);

        // No repeated locations within a route.
        std::set<std::string> visited(
            route.path.begin(), route.path.end()
            );
        assert(visited.size() == route.path.size());

        // Recalculate and verify the route distance.
        double distance = 0.0;

        for (std::size_t i = 0; i + 1 < route.path.size(); ++i)
        {
            double roadDistance = 0.0;

            assert(graph.getRoadDistance(
                route.path[i],
                route.path[i + 1],
                roadDistance
                ));

            distance += roadDistance;
        }

        assert(std::abs(distance - route.totalDistance) < 1e-9);
    }

    // Routes must be ordered from shortest to longest.
    for (std::size_t i = 1; i < routes.size(); ++i)
    {
        assert(routes[i - 1].totalDistance
               <= routes[i].totalDistance);
    }

    // Test 2: k = 1 should return only the shortest route.
    auto oneRoute = kShortestPaths(
        graph, "Pune Station", "Hadapsar", 1
        );

    assert(oneRoute.size() == 1);
    assert(oneRoute[0].valid);

    // Test 3: k = 0 should return no routes.
    auto zeroRoutes = kShortestPaths(
        graph, "Pune Station", "Hadapsar", 0
        );

    assert(zeroRoutes.empty());

    // Test 4: An invalid location should return no routes.
    auto invalidRoutes = kShortestPaths(
        graph, "Unknown Place", "Hadapsar", 3
        );

    assert(invalidRoutes.empty());

    // Test 5: Source equals destination.
    auto sameLocation = kShortestPaths(
        graph, "Pune Station", "Pune Station", 3
        );

    assert(sameLocation.size() == 1);
    assert(sameLocation[0].valid);
    assert(sameLocation[0].totalDistance == 0.0);
    assert(sameLocation[0].path.size() == 1);

    std::cout << "All alternative route tests passed!\n";

    for (std::size_t i = 0; i < routes.size(); ++i)
    {
        std::cout << "\nRoute " << i + 1 << ": ";

        for (std::size_t j = 0; j < routes[i].path.size(); ++j)
        {
            if (j > 0)
                std::cout << " -> ";

            std::cout << routes[i].path[j];
        }

        std::cout << "\nDistance: "
                  << routes[i].totalDistance << " km\n";
    }

    return 0;
}