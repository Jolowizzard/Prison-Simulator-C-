#pragma once
#include <pthread.h>

constexpr int MAX_ROOMS = 20;
constexpr int NO_EDGE = -1;

struct PrisonGraph {
    int roomCount;
    int adjacency[MAX_ROOMS][MAX_ROOMS];
    pthread_mutex_t mutex;
};

void initGraph(PrisonGraph& graph, pthread_mutexattr_t& attr);
int addRoom(PrisonGraph& graph);
void addCorridor(PrisonGraph& graph, int from, int to, int cost);
