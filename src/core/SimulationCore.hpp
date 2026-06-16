#pragma once
#include <pthread.h>
#include "RoomState.hpp"
#include "PrisonGraph.hpp"
#include <atomic>
#include "CantineState.hpp"

constexpr int MAX_PRISONERS = 100;//Space reserved for prisoners in RAM - phisical count can be smaller
constexpr int MAX_GUARDS = 10;
constexpr int MAX_JANITORS = 10;
constexpr int MAX_DOCTORS = 10;
//constexpr int ROOM_COUNT = 4; // Same as above
constexpr int MAX_ACTIVE_FIGHTS = 50;
constexpr int MAX_CANTINE_MEALS = 20;
constexpr int MAX_HOSPITAL_BEDS = 10;


enum class PrisonerStateEnum {
    IDLE,       
    MOVING,     
    FIGHTING,   
    EATING,    
    SLEEPING,
    HEALING,
    ISOLATING,
    QUEUEING
};


struct PrisonerState {
    int id;
    int aggression;
    int hunger;
    int health;
    int exhaustion;
    int currentRoom;
    bool alive;
    bool active;
    int cellId;
    
    std::atomic<PrisonerStateEnum> state; //For better gui

    pthread_mutex_t mutex;
};

struct FightState {
    bool active;       
    int attackerId;
    int defenderId;
    int roomId;         
    int durationLeft;   
    pthread_mutex_t mutex;
};

struct GuardState {
    int id;
    int currentRoom;          // Aktualny pokój (do wyświetlania)
    bool isMoving;            // Czy jest w trakcie ruchu (opcjonalne dla GUI)
    char actionInfo[32];      // Np. "PATROL", "INTERWENCJA" (do wyświetlania tekstowego)
    pthread_mutex_t mutex;    // Do synchronizacji zmian stanu
};

struct JanitorState {
    int id;
    int currentRoom;
    char actionInfo[64]; // np. "Replacing bedding", "Walking", "Idle"
    pthread_mutex_t mutex;
};

struct BeddingStats {
    int clean;   // dostępne, pościelone
    int dirty;   // brudne, czekają na wymianę
    int totalChanges;   // suma (informacyjnie)
    pthread_mutex_t mutex;
};

struct DoctorState {
    int id;
    int currentRoom;          // zwykle ROOM_HOSPITAL
    char actionInfo[64];      // np. "Healing prisoner", "Idle"

    pthread_mutex_t mutex;
};

struct HospitalState {
    int totalBeds;
    int occupiedBeds;

    sem_t freeBeds;       // liczba wolnych łóżek
    pthread_mutex_t mutex;
};

struct SimulationState {
    GuardState guards[MAX_GUARDS];
    int guardsCount;

    PrisonerState prisoners[MAX_PRISONERS];
    int prisonersCount;

    pthread_mutex_t globalMutex;

    RoomState rooms[MAX_ROOMS];
    int roomsCount;

    PrisonGraph graph;
    int numberOfCorridors;

    FightState fights[MAX_ACTIVE_FIGHTS];

    CantineState cantine; 

    int laundryRoomId;

    JanitorState janitors[MAX_JANITORS];
    int janitorsCount;

    BeddingStats beddingStats;

    HospitalState hospital;

    DoctorState doctors[MAX_DOCTORS];
    int doctorsCount;

    bool simulationRunning;
    pthread_mutex_t simMutex;
};
