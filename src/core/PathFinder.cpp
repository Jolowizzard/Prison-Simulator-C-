#include "PathFinder.hpp"
#include <limits>
#include <algorithm>

std::vector<int> PathFinder::shortestPath(
    const PrisonGraph& graph,
    int start,
    int goal
) {
    const int INF = std::numeric_limits<int>::max();

    int n = graph.roomCount;
    std::vector<int> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<bool> visited(n, false);

    dist[start] = 0;

    for (int i = 0; i < n; ++i) {
        int u = -1;
        for (int j = 0; j < n; ++j)
            if (!visited[j] && (u == -1 || dist[j] < dist[u]))
                u = j;

        if (u == -1 || dist[u] == INF)
            break;

        visited[u] = true;

        for (int v = 0; v < n; ++v) {
            int cost = graph.adjacency[u][v];
            if (cost != NO_EDGE && !visited[v]) {
                if (dist[u] + cost < dist[v]) {
                    dist[v] = dist[u] + cost;
                    prev[v] = u;
                }
            }
        }
    }

    std::vector<int> path;
    for (int at = goal; at != -1; at = prev[at])
        path.push_back(at);

    std::reverse(path.begin(), path.end());
    return path;
}

int PathFinder::edgeWeight(const PrisonGraph& graph, int from, int to){
    return graph.adjacency[from][to];
}
