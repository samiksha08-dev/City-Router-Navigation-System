#include "AlternativeRoutes.h"

#include <algorithm>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
// Makes the priority queue return the shortest candidate first.
struct RouteGreater
{
    bool operator()(const Route &a, const Route &b) const
    {
        if (a.totalDistance != b.totalDistance)
            return a.totalDistance > b.totalDistance;

        // Deterministic ordering when distances are equal.
        return a.path > b.path;
    }
};

// Checks whether a path begins with the specified root path.
bool hasSamePrefix(
    const std::vector<std::string> &path,
    const std::vector<std::string> &root)
{
    return path.size() >= root.size()
    && std::equal(root.begin(), root.end(), path.begin());
}

// Calculates distance by looking up every road in a path.
bool calculatePathDistance(
    const CityGraph &graph,
    const std::vector<std::string> &path,
    double &distance)
{
    distance = 0.0;

    for (std::size_t i = 0; i + 1 < path.size(); ++i)
    {
        double roadDistance = 0.0;

        if (!graph.getRoadDistance(
                path[i], path[i + 1], roadDistance))
        {
            return false;
        }

        distance += roadDistance;
    }

    return true;
}
}

std::vector<Route> kShortestPaths(
    const CityGraph &graph,
    const std::string &src,
    const std::string &dst,
    std::size_t k)
{
    std::vector<Route> acceptedRoutes;

    if (k == 0)
        return acceptedRoutes;

    // 1. Find the shortest route with Dijkstra's algorithm.
    Route firstRoute = graph.shortestPath(src, dst);

    if (!firstRoute.valid)
        return acceptedRoutes;

    acceptedRoutes.push_back(firstRoute);

    // Paths already accepted or generated as candidates.
    std::set<std::vector<std::string>> acceptedPaths;
    std::set<std::vector<std::string>> generatedPaths;

    acceptedPaths.insert(firstRoute.path);
    generatedPaths.insert(firstRoute.path);

    // Candidate routes are ordered by increasing distance.
    std::priority_queue<
        Route,
        std::vector<Route>,
        RouteGreater
        > candidates;

    // 2. Generate alternatives until k routes are found
    //    or no more candidates exist.
    for (std::size_t routeIndex = 1;
         routeIndex < k;
         ++routeIndex)
    {
        const Route &previousRoute = acceptedRoutes.back();

        // Try each location before the destination as a spur node.
        for (std::size_t spurIndex = 0;
             spurIndex + 1 < previousRoute.path.size();
             ++spurIndex)
        {
            // The root path runs from the source to the spur node.
            std::vector<std::string> rootPath(
                previousRoute.path.begin(),
                previousRoute.path.begin() + spurIndex + 1
                );

            std::vector<std::pair<std::string, std::string>>
                bannedEdges;

            // Ban the next road of every accepted route that
            // shares this same root path.
            for (const Route &route : acceptedRoutes)
            {
                if (route.path.size() > spurIndex + 1
                    && hasSamePrefix(route.path, rootPath))
                {
                    bannedEdges.emplace_back(
                        route.path[spurIndex],
                        route.path[spurIndex + 1]
                        );
                }
            }

            // Prevent the spur path from revisiting earlier
            // locations in the root path.
            std::vector<std::string> bannedNodes(
                rootPath.begin(),
                rootPath.end() - 1
                );

            // Find the shortest remaining route from the spur node
            // to the original destination.
            Route spurRoute = graph.shortestPath(
                rootPath.back(),
                dst,
                bannedEdges,
                bannedNodes
                );

            if (!spurRoute.valid || spurRoute.path.empty())
                continue;

            // Combine the root and spur paths.
            // Skip the first spur location because it is already
            // the last location in the root path.
            std::vector<std::string> totalPath = rootPath;

            totalPath.insert(
                totalPath.end(),
                spurRoute.path.begin() + 1,
                spurRoute.path.end()
                );

            // Do not generate duplicate paths.
            if (acceptedPaths.find(totalPath) != acceptedPaths.end()
                || generatedPaths.find(totalPath) != generatedPaths.end())
            {
                continue;
            }

            // Calculate the complete candidate distance.
            double totalDistance = 0.0;

            if (!calculatePathDistance(
                    graph, totalPath, totalDistance))
            {
                continue;
            }

            Route candidate;
            candidate.path = totalPath;
            candidate.totalDistance = totalDistance;
            candidate.valid = true;

            candidates.push(candidate);
            generatedPaths.insert(candidate.path);
        }

        // No more valid alternatives are available.
        if (candidates.empty())
            break;

        // 3. Accept the best remaining candidate.
        Route nextRoute = candidates.top();
        candidates.pop();

        acceptedRoutes.push_back(nextRoute);
        acceptedPaths.insert(nextRoute.path);
    }

    return acceptedRoutes;
}