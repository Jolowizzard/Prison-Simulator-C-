#pragma once
#include "core/SimulationCore.hpp"
#include "prisoner/PrisonerProcess.hpp"

class BehaviorStrategy{
    public:
        virtual ~BehaviorStrategy() = default;

        virtual void decide(PrisonerProcess& prisoner, SimulationState* state) = 0;
};