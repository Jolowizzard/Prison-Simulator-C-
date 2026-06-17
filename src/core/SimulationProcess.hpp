#pragma once
#include "SimulationCore.hpp"
#include <unistd.h>

class SimulationProcess {
protected:
    SimulationState* state;

    bool isSimulationRunning() const {
        pthread_mutex_lock(&state->simMutex);
        bool running = state->simulationRunning;
        pthread_mutex_unlock(&state->simMutex);
        return running;
    }

    bool waitWithShutdown(sem_t* sem) const {
        while (isSimulationRunning()) {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;

            if (sem_timedwait(sem, &ts) == 0)
                return true;
        }
        return false;
    }

    void interruptibleSleep(int seconds){
        for (int i = 0; i < seconds * 10; ++i) { 
            if (!isSimulationRunning()) return; 
            usleep(100000);
        }
    }

public:
    explicit SimulationProcess(SimulationState* state)
        : state(state) {}

    virtual ~SimulationProcess() = default;

    virtual void run() = 0;

};