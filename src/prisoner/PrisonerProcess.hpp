#pragma once
#include "SimulationCore.hpp"
#include "RoomManager.hpp"
#include "FightManager.hpp"
#include "PathFinder.hpp"
#include "SimulationProcess.hpp"

class PrisonerProcess : public SimulationProcess{
public:
    PrisonerProcess(int id, SimulationState* state);
    void run() override;
    int chooseOpponent();
    void goToRoom(RoomType t);
    void goToCell();
    int getId();

private:
    int id;
    std::vector<int> currentPath;
    int moveTimer = 0;
    bool isTraversing = false;
    int currentFightIndex = -1;

    //Funkcja wywoływana raz na iterację pętli życia. Symuluje procesy życiowe wieźnia i uaktualnia paramtery głodu i agresji
    void think();
    void act();
    void move();
    int getTravelTime(int fromRoom, int toRoom);
    void goEat();
    void wander();
    void goSleep();
    void goHospital();
};
