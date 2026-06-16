#pragma once
#include "SimulationCore.hpp"
#include "SimulationProcess.hpp"

class DoctorProcess : public SimulationProcess{
public:
    DoctorProcess(int id, SimulationState* state);
    void run() override;

private:
    int id;
    void healOne();
};
