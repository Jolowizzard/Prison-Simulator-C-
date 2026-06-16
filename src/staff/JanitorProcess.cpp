#include "JanitorProcess.hpp"
#include "PathFinder.hpp"
#include <stdlib.h>
#include <string>
#include <cstring>

JanitorProcess::JanitorProcess(int id, SimulationState* state)
: id(id), SimulationProcess(state){}

void JanitorProcess::run() {
    srand(time(nullptr) ^ getpid());

    JanitorState& me = state->janitors[id];

    while (isSimulationRunning()) {

        bool didWork = false;

        for (int i = 0; i < state->roomsCount; i++) {

            if (state->rooms[i].type != RoomType::CELL)
                continue;

            RoomState& cell = state->rooms[i];

            if (sem_trywait(&cell.dirtyBedding) == 0) {

                didWork = true;

                int startRoom;
                pthread_mutex_lock(&me.mutex);
                startRoom = me.currentRoom;
                strcpy(me.actionInfo, "Going to cell ");
                pthread_mutex_unlock(&me.mutex);

                auto path = PathFinder::shortestPath(
                    state->graph,
                    startRoom,
                    i
                );

                for (size_t p = 1; p < path.size(); p++) {
                    int from = path[p - 1];
                    int to   = path[p];

                    int travelTime =
                        PathFinder::edgeWeight(state->graph, from, to);

                    sleep(travelTime);

                    pthread_mutex_lock(&me.mutex);
                    me.currentRoom = to;
                    pthread_mutex_unlock(&me.mutex);
                }

                pthread_mutex_lock(&me.mutex);
                strcpy(me.actionInfo, "Cleaning bedding ");
                pthread_mutex_unlock(&me.mutex);

                sleep(2 + rand() % 3);

                sem_post(&cell.cleanBedding);

                pthread_mutex_lock(&state->beddingStats.mutex);
                state->beddingStats.dirty--;
                state->beddingStats.clean++;
                state->beddingStats.totalChanges++;
                pthread_mutex_unlock(&state->beddingStats.mutex);
            }
        }

        if (!didWork) {
            pthread_mutex_lock(&me.mutex);
            strcpy(me.actionInfo, "Idle");
            pthread_mutex_unlock(&me.mutex);
            sleep(1);
        }
    }
}
