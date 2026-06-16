#pragma once
#include "SimulationCore.hpp"
#include "SimulationProcess.hpp"

class CookProcess : public SimulationProcess{
public:
    CookProcess(int cookId, SimulationState* state, int minCookingTime, int maxCookingTime);
    void run() override;

private:
    int id;
    int minCookingTime;
    int maxCookingTime;
};