#pragma once
#include <vector>
#include "PrisonGraph.hpp"

class PathFinder {
public:
    static std::vector<int> shortestPath(
        const PrisonGraph& graph,
        int start,
        int goal
    );
    static int edgeWeight(const PrisonGraph& graph, int from, int to);
};
