#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <cstring>
#include <vector>

#include "core/SimulationCore.hpp"
#include "core/PrisonGraph.hpp"
#include "prisoner/PrisonerProcess.hpp"
#include "rooms/RoomState.hpp"
#include "rooms/RoomType.hpp"
#include "gui/SimulationGUI.hpp"
#include "GuardProcess.hpp"
#include "CookProcess.hpp"
#include "JanitorProcess.hpp"
#include "DoctorProcess.hpp"

// Nowe nagłówki
#include "PrisonConfig.hpp"
#include "ConfigLoader.hpp"

template <typename ProcessType>
void createProcesses(int count, SimulationState* state) {
    for (int i = 0; i < count; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            ProcessType p(i, state);
            p.run();
            _exit(0);
        }
    }
}

void applyConfigToState(SimulationState* state, const PrisonConfig& cfg, pthread_mutexattr_t& attr) {
    
    // 1. Przepisanie liczników
    state->roomsCount = cfg.rooms.size();
    state->numberOfCorridors = cfg.corridors.size();
    state->prisonersCount = cfg.numPrisoners;
    state->guardsCount = cfg.numGuards;
    state->janitorsCount = cfg.numJanitors;
    state->doctorsCount = cfg.numDoctors;
    
    state->laundryRoomId = cfg.laundryRoomId;

    // 2. Inicjalizacja POKOI
    int cellCount = 0;
    int firstCellId = -1;

    for (const auto& roomCfg : cfg.rooms) {
        RoomState& r = state->rooms[roomCfg.id];
        r.type = roomCfg.type;
        r.capacity = roomCfg.capacity;
        r.occupantCount = 0;
        
        pthread_mutex_init(&r.mutex, &attr);

        if (r.type == RoomType::CELL) {
            sem_init(&r.cleanBedding, 1, r.capacity);
            sem_init(&r.dirtyBedding, 1, 0);
            
            cellCount++;
            if (firstCellId == -1) firstCellId = roomCfg.id;
        } 
        else if (r.type == RoomType::CANTINE) {
            state->cantine.currentMeals = 0;
            state->cantine.qeueSize = 0;
            state->cantine.ticketDispenser = 0;
            state->cantine.nowServing = 0;
            pthread_mutex_init(&state->cantine.mutex, &attr);
            sem_init(&state->cantine.mealsAvailable, 1, 0);
            sem_init(&state->cantine.freeSlots, 1, MAX_MEALS);
            sem_init(&state->cantine.qeueLine, 1, MAX_QUEUE);
        }
        else if (r.type == RoomType::HOSPITAL) {
            state->hospital.totalBeds = r.capacity;
            state->hospital.occupiedBeds = 0;
            pthread_mutex_init(&state->hospital.mutex, &attr);
            sem_init(&state->hospital.freeBeds, 1, state->hospital.totalBeds);
        }
    }

    // 3. Inicjalizacja POŚCIELI (statystyki globalne)
    pthread_mutex_init(&(state->beddingStats.mutex), &attr);
    int totalCapacityCells = 0;
    for(const auto& rc : cfg.rooms) if(rc.type == RoomType::CELL) totalCapacityCells += rc.capacity;
    
    state->beddingStats.clean = totalCapacityCells;
    state->beddingStats.dirty = 0;
    state->beddingStats.totalChanges = 0;

    // 4. Inicjalizacja GRAFU (Korytarze)
    initGraph(state->graph, attr);
    for(int i = 0; i < state->roomsCount; i++) {
        addRoom(state->graph);
    }
    for(const auto& corr : cfg.corridors) {
        addCorridor(state->graph, corr.from, corr.to, corr.distance);
    }

    // 5. Inicjalizacja WIĘŹNIÓW
    for (int i = 0; i < cfg.numPrisoners; ++i) {
        auto& p = state->prisoners[i];
        p.id = i;
        p.aggression = 0;
        p.hunger = 0;
        p.health = 100;
        p.exhaustion = 0;
        p.alive = true;
        p.active = true;
        
        if (cellCount > 0) {
            int targetCellIndex = i % cellCount;
            int currentC = 0;
            for(const auto& r : cfg.rooms) {
                if(r.type == RoomType::CELL) {
                    if(currentC == targetCellIndex) {
                        p.cellId = r.id;
                        break;
                    }
                    currentC++;
                }
            }
        } else {
            p.cellId = 0; // Fallback
        }

        p.currentRoom = p.cellId;
        p.state.store(PrisonerStateEnum::IDLE);
        pthread_mutex_init(&p.mutex, &attr);
        
        // Wsadzamy do pokoju logicznie
        RoomManager::enterRoom(state, i, p.currentRoom);
    }

    // 6. Inicjalizacja STRAŻNIKÓW
    for (int i = 0; i < cfg.numGuards; ++i) {
        state->guards[i].id = i;
        state->guards[i].currentRoom = (cfg.lobbyRoomId != -1) ? cfg.lobbyRoomId : 0;
        state->guards[i].isMoving = false;
        sprintf(state->guards[i].actionInfo, "PATROL");
        pthread_mutex_init(&state->guards[i].mutex, &attr);
    }

    // 7. Inicjalizacja WOŹNYCH
    for(int i = 0; i < cfg.numJanitors; i++){
        state->janitors[i].id = i;
        state->janitors[i].currentRoom = (cfg.laundryRoomId != -1) ? cfg.laundryRoomId : 0;
        strcpy(state->janitors[i].actionInfo, "Idle");
        pthread_mutex_init(&state->janitors[i].mutex, &attr);
    }

    // 8. Inicjalizacja LEKARZY
    for (int i = 0; i < cfg.numDoctors; i++) {
        DoctorState& d = state->doctors[i];
        d.id = i;
        d.currentRoom = (cfg.hospitalRoomId != -1) ? cfg.hospitalRoomId : 0;
        strcpy(d.actionInfo, "Idle");
        pthread_mutex_init(&d.mutex, &attr);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "[MAIN] Start symulacji\n";

    // 1. Wczytanie konfiguracji
    PrisonConfig config;
    if (argc > 1) {
        config = ConfigLoader::loadFromFile(argv[1]);
    } else {
        std::cout << "[MAIN] Brak pliku konfiguracyjnego. Uzycie domyslnych ustawien.\n";
        std::cout << "       Uzycie: ./prison_simulation [sciezka_do_config.txt]\n";
        config = ConfigLoader::loadDefaults();
    }

    // 2. Przygotowanie atrybutów mutexów
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    // 3. Pamięć współdzielona (mmap)
    auto* state = static_cast<SimulationState*>(
        mmap(nullptr, sizeof(SimulationState), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)
    );

    if (state == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    memset(state, 0, sizeof(SimulationState));

    // Inicjalizacja głównego mutexa symulacji
    state->simulationRunning = true;
    pthread_mutex_init(&state->simMutex, &attr);

    // 4. Aplikacja konfiguracji do stanu symulacji
    applyConfigToState(state, config, attr);

    // 5. Tworzenie procesów (Workerów)
    createProcesses<GuardProcess>(config.numGuards, state);
    createProcesses<JanitorProcess>(config.numJanitors, state);
    createProcesses<DoctorProcess>(config.numDoctors, state);

    for (int i = 0; i < config.numCooks; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            CookProcess c(i, state, 5, 8);
            c.run();
            //std::cout << "[DEBUG] Kucharz " << i << " zakończył. CantineID: " << "\n";
            _exit(0);
        }
    }

    createProcesses<PrisonerProcess>(config.numPrisoners, state);

    // 6. GUI
    SimulationGUI gui(state);
    gui.run();

    // 7. Zamykanie symulacji
    state->simulationRunning = false;
    
    sem_post(&state->cantine.mealsAvailable);
    sem_post(&state->cantine.freeSlots);

    while (wait(nullptr) > 0);

    sem_destroy(&state->hospital.freeBeds);
    sem_destroy(&state->cantine.mealsAvailable);
    sem_destroy(&state->cantine.freeSlots);
    sem_destroy(&state->cantine.qeueLine);
    pthread_mutex_destroy(&state->hospital.mutex);
    pthread_mutex_destroy(&state->simMutex);

    munmap(state, sizeof(SimulationState));
    std::cout << "[MAIN] Koniec symulacji\n";

    return 0;
}