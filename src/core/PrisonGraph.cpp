#include "PrisonGraph.hpp"

void initGraph(PrisonGraph& graph, pthread_mutexattr_t& attr) {
    graph.roomCount = 0;
    pthread_mutex_init(&graph.mutex, &attr);

    for (int i = 0; i < MAX_ROOMS; ++i)
        for (int j = 0; j < MAX_ROOMS; ++j)
            graph.adjacency[i][j] = NO_EDGE;
}

int addRoom(PrisonGraph& graph) {
    pthread_mutex_lock(&graph.mutex);
    int id = graph.roomCount++;
    graph.adjacency[id][id] = 0;
    pthread_mutex_unlock(&graph.mutex);
    return id;
}

void addCorridor(PrisonGraph& graph, int from, int to, int cost) {
    pthread_mutex_lock(&graph.mutex);
    graph.adjacency[from][to] = cost;
    graph.adjacency[to][from] = cost;
    pthread_mutex_unlock(&graph.mutex);
}
