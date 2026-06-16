#include "CookProcess.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

CookProcess::CookProcess(int id, SimulationState* state, int minCookingTime, int maxCookingTime)
    : id(id), SimulationProcess(state),  minCookingTime(minCookingTime), maxCookingTime(maxCookingTime){}

void CookProcess::run(){
    srand(getpid());

    while (isSimulationRunning()) {
        // symulacja czasu gotowania
        interruptibleSleep((rand() % (maxCookingTime - minCookingTime)) + minCookingTime);
        // czekaj aż będzie miejsce
        waitWithShutdown(&state->cantine.freeSlots);

        pthread_mutex_lock(&state->cantine.mutex);
        state->cantine.currentMeals++;
        pthread_mutex_unlock(&state->cantine.mutex);

        // poinformuj, że pojawił się posiłek
        sem_post(&state->cantine.mealsAvailable);
    }
}