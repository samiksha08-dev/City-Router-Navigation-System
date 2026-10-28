#pragma once
#include "Route.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

class CityGraph {
public:
    CityGraph();   // loads the 10 cities and 20 roads
    void addRoad(const std::string& a, const std::string& b, double dist);
    bool hasLocation(const std::string& name) const;
    std::vector<std::string> locations() const;
    Route shortestPath(const std::string& src, const std::string& dst) const;
    Route shortestPath(const std::string& src, const std::string& dst,
        const std::vector<std::pair<std::string,std::string>>& bannedEdges,
        const std::vector<std::string>& bannedNodes) const;
private:
    std::vector<std::string> names_;
    std::map<std::string,int> index_;
    std::vector<std::vector<std::pair<int,double>>> adj_;
};
