#pragma once
#include "SimulationCore.hpp"
#include "SimulationProcess.hpp"
#include <vector>

class JanitorProcess : public SimulationProcess{
    int id;
    int currentRoom;
    std::vector<int> currentPath;
public:
    JanitorProcess(int id, SimulationState* state);
    void run() override;
};
