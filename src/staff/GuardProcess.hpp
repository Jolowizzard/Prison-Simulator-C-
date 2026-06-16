#ifndef GUARDPROCESS_HPP
#define GUARDPROCESS_HPP

#include "SimulationCore.hpp"
#include "SimulationProcess.hpp"
#include <vector>

class GuardProcess : public SimulationProcess{
public:
    GuardProcess(int id, SimulationState* state);
    void run() override;

private:
    int id;
    
    // Zmienne nawigacyjne
    int currentRoom;
    std::vector<int> currentPath;
    bool isTraversing = false;
    int moveTimer = 0;

    // Logika zachowań
    void think();
    void move();
    
    // Metody pomocnicze
    int scanForFights(); // Zwraca ID walki lub -1
    void setDestination(int targetRoomId);
    void wander(); // Losowy patrol
    int getTravelTime(int from, int to);
};

#endif // GUARDPROCESS_HPP