#include "CityGraph.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <set>
#include <functional>

/**
 * @brief Constructs the Pune city road network with 10 locations and 20 bidirectional roads.
 * 
 * Locations:
 * Pune Station, Shivajinagar, Swargate, Deccan, Kothrud,
 * Hinjewadi, Pune Airport, Viman Nagar, Koregaon Park, Hadapsar
 */
CityGraph::CityGraph() {
    // 20 Bidirectional Roads with realistic distances in kilometers
    addRoad("Pune Station",  "Koregaon Park",  3.0);  // Road 1
    addRoad("Pune Station",  "Shivajinagar",   4.5);  // Road 2
    addRoad("Pune Station",  "Swargate",       4.0);  // Road 3
    addRoad("Pune Station",  "Deccan",         5.0);  // Road 4
    addRoad("Pune Station",  "Viman Nagar",    8.0);  // Road 5
    addRoad("Shivajinagar",  "Deccan",         2.5);  // Road 6
    addRoad("Shivajinagar",  "Kothrud",        6.5);  // Road 7
    addRoad("Shivajinagar",  "Hinjewadi",     14.0);  // Road 8
    addRoad("Shivajinagar",  "Koregaon Park",  6.0);  // Road 9
    addRoad("Swargate",      "Deccan",         4.0);  // Road 10
    addRoad("Swargate",      "Hadapsar",      10.0);  // Road 11
    addRoad("Swargate",      "Kothrud",        9.0);  // Road 12
    addRoad("Deccan",        "Kothrud",        4.5);  // Road 13
    addRoad("Kothrud",       "Hinjewadi",     11.0);  // Road 14
    addRoad("Pune Airport",  "Viman Nagar",    3.5);  // Road 15
    addRoad("Pune Airport",  "Koregaon Park",  9.0);  // Road 16
    addRoad("Pune Airport",  "Hadapsar",      12.0);  // Road 17
    addRoad("Viman Nagar",   "Koregaon Park",  5.5);  // Road 18
    addRoad("Koregaon Park", "Hadapsar",       7.5);  // Road 19
    addRoad("Viman Nagar",   "Hadapsar",       9.5);  // Road 20
}

/**
 * @brief Adds a bidirectional road between two locations with the given distance.
 * 
 * Automatically registers unknown locations into names_, index_, and adj_.
 * Adds the edge in both directions to maintain an undirected graph.
 */
void CityGraph::addRoad(const std::string& a, const std::string& b, double dist) {
    // Auto-create location 'a' if not previously seen
    if (index_.find(a) == index_.end()) {
        int newIdx = static_cast<int>(names_.size());
        index_[a] = newIdx;
        names_.push_back(a);
        adj_.emplace_back();
    }

    // Auto-create location 'b' if not previously seen
    if (index_.find(b) == index_.end()) {
        int newIdx = static_cast<int>(names_.size());
        index_[b] = newIdx;
        names_.push_back(b);
        adj_.emplace_back();
    }

    int u = index_[a];
    int v = index_[b];

    // Every road is bidirectional (undirected graph)
    adj_[u].push_back({v, dist});
    adj_[v].push_back({u, dist});
}

/**
 * @brief Checks if a location exists in the graph.
 */
bool CityGraph::hasLocation(const std::string& name) const {
    return index_.find(name) != index_.end();
}

/**
 * @brief Returns all registered location names in index order.
 */
std::vector<std::string> CityGraph::locations() const {
    return names_;
}

/**
 * @brief Standard shortest path query delegating to the overload with empty ban lists.
 * This guarantees a single, maintainable Dijkstra implementation.
 */
Route CityGraph::shortestPath(const std::string& src, const std::string& dst) const {
    return shortestPath(src, dst, {}, {});
}

/**
 * @brief Dijkstra's shortest path algorithm supporting banned edges and banned nodes.
 * 
 * Used by Member 3 for Yen's K-shortest paths algorithm:
 * - Banned edges are avoided in both directions.
 * - Banned nodes cannot be used as intermediate vertices or endpoints.
 * 
 * Algorithm Steps (Viva Reference):
 * 1. Input Validation: Check if source and destination exist in the graph.
 * 2. Ban Validation: If source or destination is a banned node, return valid = false.
 * 3. Base Case: If source == destination, return a 1-node path with 0 distance.
 * 4. Initialization:
 *    - dist[] vector initialized to infinity (std::numeric_limits<double>::infinity()).
 *    - parent[] vector initialized to -1 (to reconstruct path predecessors).
 *    - std::priority_queue (min-heap) ordered by tentative distance.
 * 5. Greedy Exploration:
 *    - Pop vertex u with minimum tentative distance from priority queue.
 *    - If u == destination, shortest path is found; break early.
 *    - Discard stale heap entries where current popped distance > dist[u].
 * 6. Edge Relaxation:
 *    - For each neighbor v of u:
 *      - Skip if v is in bannedNodes.
 *      - Skip if edge (u, v) is in bannedEdges.
 *      - If dist[u] + weight(u, v) < dist[v]:
 *        Update dist[v] and record parent[v] = u.
 *        Push (dist[v], v) into min-heap.
 * 7. Path Reconstruction:
 *    - If dist[dst] is infinity or parent[dst] == -1, destination is unreachable.
 *    - Otherwise, backtrack from dst using parent[] array to src, reverse path, and return.
 */
Route CityGraph::shortestPath(const std::string& src, const std::string& dst,
    const std::vector<std::pair<std::string,std::string>>& bannedEdges,
    const std::vector<std::string>& bannedNodes) const {

    // Step 1: Validate existence of source and destination
    auto itSrc = index_.find(src);
    auto itDst = index_.find(dst);
    if (itSrc == index_.end() || itDst == index_.end()) {
        return Route{}; // Invalid location -> valid = false
    }

    int srcIdx = itSrc->second;
    int dstIdx = itDst->second;

    // Step 2: Convert banned nodes to integer set for O(log B) lookups
    std::set<int> bannedNodeSet;
    for (const auto& node : bannedNodes) {
        auto it = index_.find(node);
        if (it != index_.end()) {
            bannedNodeSet.insert(it->second);
        }
    }

    // If source or destination is banned, no valid path can exist
    if (bannedNodeSet.find(srcIdx) != bannedNodeSet.end() ||
        bannedNodeSet.find(dstIdx) != bannedNodeSet.end()) {
        return Route{}; // valid = false
    }

    // Step 3: Handle trivial case where source and destination are identical
    if (srcIdx == dstIdx) {
        Route route;
        route.path = {src};
        route.totalDistance = 0.0;
        route.valid = true;
        return route;
    }

    // Step 4: Convert banned edges to index pairs (bidirectional lookup)
    std::set<std::pair<int, int>> bannedEdgeSet;
    for (const auto& edge : bannedEdges) {
        auto itU = index_.find(edge.first);
        auto itV = index_.find(edge.second);
        if (itU != index_.end() && itV != index_.end()) {
            int u = itU->second;
            int v = itV->second;
            // Banned edges apply in both directions
            bannedEdgeSet.insert(std::make_pair(u, v));
            bannedEdgeSet.insert(std::make_pair(v, u));
        }
    }

    // Step 5: Initialize Dijkstra data structures
    const size_t numVertices = names_.size();
    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(numVertices, INF);
    std::vector<int> parent(numVertices, -1);

    // Min-heap storing pairs of (distance, vertex_index)
    using PqElement = std::pair<double, int>;
    std::priority_queue<PqElement, std::vector<PqElement>, std::greater<PqElement>> pq;

    // Start from source vertex
    dist[srcIdx] = 0.0;
    pq.push(std::make_pair(0.0, srcIdx));

    // Step 6: Dijkstra main relaxation loop
    while (!pq.empty()) {
        PqElement topElement = pq.top();
        double currentDist = topElement.first;
        int u = topElement.second;
        pq.pop();

        // Early termination: Once destination vertex is popped, its shortest distance is finalized
        if (u == dstIdx) {
            break;
        }

        // Lazy deletion: ignore stale entries in the priority queue
        if (currentDist > dist[u]) {
            continue;
        }

        // Traverse all adjacent roads from vertex u
        for (const auto& edge : adj_[u]) {
            int v = edge.first;
            double weight = edge.second;

            // Check if neighbor vertex is banned
            if (bannedNodeSet.find(v) != bannedNodeSet.end()) {
                continue;
            }

            // Check if bidirectional road (u, v) is banned
            if (bannedEdgeSet.find(std::make_pair(u, v)) != bannedEdgeSet.end()) {
                continue;
            }

            // Relaxation step: has a shorter path to v via u been found?
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
                pq.push(std::make_pair(dist[v], v));
            }
        }
    }

    // Step 7: Check if destination was unreachable
    if (dist[dstIdx] == INF || parent[dstIdx] == -1) {
        return Route{}; // Unreachable -> valid = false
    }

    // Step 8: Reconstruct path by walking parent[] back from destination to source
    std::vector<std::string> path;
    for (int curr = dstIdx; curr != -1; curr = parent[curr]) {
        path.push_back(names_[curr]);
    }
    std::reverse(path.begin(), path.end());

    Route route;
    route.path = std::move(path);
    route.totalDistance = dist[dstIdx];
    route.valid = true;
    return route;
}
