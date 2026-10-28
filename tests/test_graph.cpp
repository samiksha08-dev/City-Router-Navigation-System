#include "CityGraph.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// Helper to compare floating-point distances with tolerance
static inline bool approxEqual(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}

int main() {
    CityGraph graph;

    // =========================================================================
    // Test 1: Normal route Pune Station -> Pune Airport
    // Hand Calculation:
    // Path: Pune Station -> Viman Nagar -> Pune Airport
    // Leg 1: Pune Station to Viman Nagar = 8.0 km
    // Leg 2: Viman Nagar to Pune Airport = 3.5 km
    // Total Distance = 8.0 + 3.5 = 11.5 km
    // Alternative routes:
    //   - via Koregaon Park: Pune Station -> Koregaon Park (3.0) -> Pune Airport (9.0) = 12.0 km
    //   - via Hadapsar: Pune Station -> Koregaon Park (3.0) -> Hadapsar (7.5) -> Pune Airport (12.0) = 22.5 km
    // Thus, 11.5 km is the strictly minimal shortest path.
    // =========================================================================
    {
        Route r = graph.shortestPath("Pune Station", "Pune Airport");
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 11.5));
        std::vector<std::string> expected = {"Pune Station", "Viman Nagar", "Pune Airport"};
        assert(r.path == expected);
    }

    // =========================================================================
    // Test 2: Banned edge Pune Station - Viman Nagar
    // Hand Calculation:
    // With edge (Pune Station, Viman Nagar) banned, the 11.5 km route is blocked.
    // Two alternative paths tie for the new shortest distance of 12.0 km:
    //   1. Pune Station -> Koregaon Park (3.0) -> Pune Airport (9.0) = 12.0 km
    //   2. Pune Station -> Koregaon Park (3.0) -> Viman Nagar (5.5) -> Pune Airport (3.5) = 12.0 km
    // Prompt requirement: Assert on the distance only, not the exact path, because two paths tie.
    // =========================================================================
    {
        std::vector<std::pair<std::string, std::string>> bannedEdges = {
            {"Pune Station", "Viman Nagar"}
        };
        Route r = graph.shortestPath("Pune Station", "Pune Airport", bannedEdges, {});
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 12.0));
    }

    // =========================================================================
    // Test 3: Banned node Viman Nagar
    // Hand Calculation:
    // With vertex "Viman Nagar" completely excluded, neither the 8.0 km nor 5.5 km
    // roads touching Viman Nagar can be used.
    // The only remaining shortest route to Pune Airport is:
    //   Pune Station -> Koregaon Park (3.0) -> Pune Airport (9.0) = 12.0 km
    // Path: ["Pune Station", "Koregaon Park", "Pune Airport"]
    // =========================================================================
    {
        std::vector<std::string> bannedNodes = {"Viman Nagar"};
        Route r = graph.shortestPath("Pune Station", "Pune Airport", {}, bannedNodes);
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 12.0));
        std::vector<std::string> expected = {"Pune Station", "Koregaon Park", "Pune Airport"};
        assert(r.path == expected);
    }

    // =========================================================================
    // Test 4: Hinjewadi -> Swargate
    // Hand Calculation:
    // Option A: Hinjewadi -> Kothrud (11.0) -> Deccan (4.5) -> Swargate (4.0)
    //           Total = 11.0 + 4.5 + 4.0 = 19.5 km
    // Option B: Hinjewadi -> Kothrud (11.0) -> Swargate (9.0 direct)
    //           Total = 11.0 + 9.0 = 20.0 km
    // Option C: Hinjewadi -> Shivajinagar (14.0) -> Deccan (2.5) -> Swargate (4.0)
    //           Total = 14.0 + 2.5 + 4.0 = 20.5 km
    // Therefore, the path via Deccan (19.5 km) beats the direct Kothrud-Swargate road (20.0 km).
    // =========================================================================
    {
        Route r = graph.shortestPath("Hinjewadi", "Swargate");
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 19.5));
        std::vector<std::string> expected = {"Hinjewadi", "Kothrud", "Deccan", "Swargate"};
        assert(r.path == expected);
    }

    // =========================================================================
    // Test 5: Hadapsar -> Deccan
    // Hand Calculation:
    // Option A: Hadapsar -> Swargate (10.0) -> Deccan (4.0) = 14.0 km
    // Option B: Hadapsar -> Koregaon Park (7.5) -> Pune Station (3.0) -> Deccan (5.0) = 15.5 km
    // Option C: Hadapsar -> Koregaon Park (7.5) -> Shivajinagar (6.0) -> Deccan (2.5) = 16.0 km
    // Shortest path is Hadapsar -> Swargate -> Deccan with distance 14.0 km.
    // =========================================================================
    {
        Route r = graph.shortestPath("Hadapsar", "Deccan");
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 14.0));
        std::vector<std::string> expected = {"Hadapsar", "Swargate", "Deccan"};
        assert(r.path == expected);
    }

    // =========================================================================
    // Test 6: Hadapsar -> Pune Airport
    // Hand Calculation:
    // Option A: Hadapsar -> Pune Airport (direct road) = 12.0 km
    // Option B: Hadapsar -> Viman Nagar (9.5) -> Pune Airport (3.5) = 13.0 km
    // Option C: Hadapsar -> Koregaon Park (7.5) -> Pune Airport (9.0) = 16.5 km
    // Direct road of 12.0 km is strictly shorter than via Viman Nagar (13.0 km).
    // =========================================================================
    {
        Route r = graph.shortestPath("Hadapsar", "Pune Airport");
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 12.0));
        std::vector<std::string> expected = {"Hadapsar", "Pune Airport"};
        assert(r.path == expected);
    }

    // =========================================================================
    // Test 7: Same source and destination
    // Must return valid = true, single-node path, distance 0.0.
    // =========================================================================
    {
        Route r = graph.shortestPath("Pune Station", "Pune Station");
        assert(r.valid);
        assert(approxEqual(r.totalDistance, 0.0));
        assert(r.path.size() == 1);
        assert(r.path[0] == "Pune Station");
    }

    // =========================================================================
    // Test 8: Invalid location name
    // Test unknown source, unknown destination, and both unknown.
    // =========================================================================
    {
        // Unknown source
        Route r1 = graph.shortestPath("NonExistentCity", "Pune Station");
        assert(!r1.valid);
        assert(r1.path.empty());

        // Unknown destination
        Route r2 = graph.shortestPath("Pune Station", "Atlantis");
        assert(!r2.valid);
        assert(r2.path.empty());

        // Both unknown
        Route r3 = graph.shortestPath("NonExistentA", "NonExistentB");
        assert(!r3.valid);
        assert(r3.path.empty());
    }

    // =========================================================================
    // Test 9: Unreachable node (Pune graph plus an isolated component)
    // CityGraph isolatedGraph runs the constructor, loading the 10 Pune locations
    // and 20 roads. We then add two new locations ("Remote Hill 1" and
    // "Remote Hill 2") connected only to each other via addRoad. This creates a
    // Pune graph plus an isolated component disconnected from the Pune network,
    // while keeping the main graph instance unmodified.
    // =========================================================================
    {
        CityGraph isolatedGraph;
        // Add two isolated locations connected to each other, but not to Pune
        isolatedGraph.addRoad("Remote Hill 1", "Remote Hill 2", 15.0);

        assert(isolatedGraph.hasLocation("Remote Hill 1"));
        assert(isolatedGraph.hasLocation("Remote Hill 2"));

        // Path between Pune Station and Remote Hill 1 must be unreachable
        Route rUnreachable = isolatedGraph.shortestPath("Pune Station", "Remote Hill 1");
        assert(!rUnreachable.valid);
        assert(rUnreachable.path.empty());

        // Within the isolated component, path is valid
        Route rComponent = isolatedGraph.shortestPath("Remote Hill 1", "Remote Hill 2");
        assert(rComponent.valid);
        assert(approxEqual(rComponent.totalDistance, 15.0));

        // Verify main graph remains unaffected
        assert(!graph.hasLocation("Remote Hill 1"));
    }

    // =========================================================================
    // Test 10: Banned source or banned destination node returns valid = false
    // =========================================================================
    {
        // Source is banned
        Route rSrcBanned = graph.shortestPath("Pune Station", "Pune Airport", {}, {"Pune Station"});
        assert(!rSrcBanned.valid);

        // Destination is banned
        Route rDstBanned = graph.shortestPath("Pune Station", "Pune Airport", {}, {"Pune Airport"});
        assert(!rDstBanned.valid);

        // Both source and destination banned
        Route rBothBanned = graph.shortestPath("Pune Station", "Pune Airport", {}, {"Pune Station", "Pune Airport"});
        assert(!rBothBanned.valid);

        // Same source and destination when that node is banned
        Route rSameBanned = graph.shortestPath("Pune Station", "Pune Station", {}, {"Pune Station"});
        assert(!rSameBanned.valid);
    }

    // =========================================================================
    // Test 11: All 20 roads are bidirectional
    // Verify distance A->B equals B->A for every road defined in the requirements.
    // =========================================================================
    {
        struct TestRoad {
            std::string u;
            std::string v;
            double distance;
        };

        std::vector<TestRoad> allRoads = {
            {"Pune Station",  "Koregaon Park",  3.0},
            {"Pune Station",  "Shivajinagar",   4.5},
            {"Pune Station",  "Swargate",       4.0},
            {"Pune Station",  "Deccan",         5.0},
            {"Pune Station",  "Viman Nagar",    8.0},
            {"Shivajinagar",  "Deccan",         2.5},
            {"Shivajinagar",  "Kothrud",        6.5},
            {"Shivajinagar",  "Hinjewadi",     14.0},
            {"Shivajinagar",  "Koregaon Park",  6.0},
            {"Swargate",      "Deccan",         4.0},
            {"Swargate",      "Hadapsar",      10.0},
            {"Swargate",      "Kothrud",        9.0},
            {"Deccan",        "Kothrud",        4.5},
            {"Kothrud",       "Hinjewadi",     11.0},
            {"Pune Airport",  "Viman Nagar",    3.5},
            {"Pune Airport",  "Koregaon Park",  9.0},
            {"Pune Airport",  "Hadapsar",      12.0},
            {"Viman Nagar",   "Koregaon Park",  5.5},
            {"Koregaon Park", "Hadapsar",       7.5},
            {"Viman Nagar",   "Hadapsar",       9.5}
        };

        assert(allRoads.size() == 20);

        for (const auto& road : allRoads) {
            Route forward = graph.shortestPath(road.u, road.v);
            Route backward = graph.shortestPath(road.v, road.u);

            assert(forward.valid);
            assert(backward.valid);
            // Distance forward must equal distance backward
            assert(approxEqual(forward.totalDistance, backward.totalDistance));
            // Shortest distance cannot exceed direct road distance
            assert(forward.totalDistance <= road.distance + 1e-9);
        }
    }

    std::cout << "All tests passed\n";
    return 0;
}
