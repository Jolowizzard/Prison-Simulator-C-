#include "DoctorProcess.hpp"
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <cstring>

DoctorProcess::DoctorProcess(int id, SimulationState* state)
    : id(id), SimulationProcess(state) {}

void DoctorProcess::run() {
    srand(time(nullptr) ^ getpid());

    while (isSimulationRunning()) {
        healOne();
        interruptibleSleep(1);
    }
}

void DoctorProcess::healOne() {
    HospitalState& h = state->hospital;
    DoctorState& doctor = state->doctors[id];
    pthread_mutex_lock(&h.mutex);
    if (h.occupiedBeds == 0) {
        pthread_mutex_unlock(&h.mutex);
        return;
    }
    pthread_mutex_unlock(&h.mutex);

    // znajdź więźnia w stanie HEALING
    for (int i = 0; i < state->prisonersCount; i++) {
        PrisonerState& p = state->prisoners[i];
        
        pthread_mutex_lock(&p.mutex);
        if (p.state == PrisonerStateEnum::HEALING && p.health < 100) {
            pthread_mutex_lock(&doctor.mutex);
            strcpy(doctor.actionInfo, "Healing prisoner");
            pthread_mutex_unlock(&doctor.mutex);
            int heal = (rand() % 10) + 5;
            p.health = std::min(100, p.health + heal);

            if (p.health == 100) {
                p.state = PrisonerStateEnum::IDLE;

                pthread_mutex_lock(&h.mutex);
                h.occupiedBeds--;
                pthread_mutex_unlock(&h.mutex);

                sem_post(&h.freeBeds);
            }
            pthread_mutex_unlock(&p.mutex);

            sleep(2); // czas leczenia
            return;
        }
        pthread_mutex_unlock(&p.mutex);

        pthread_mutex_lock(&doctor.mutex);
        strcpy(doctor.actionInfo, "Idle");
        pthread_mutex_unlock(&doctor.mutex);
    }
}
