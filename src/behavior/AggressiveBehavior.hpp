#include "BehaviorStrategy.hpp"

class AggressiveBehavior : public BehaviorStrategy{
public:
    void decide(PrisonerProcess& prisoner, SimulationState* state) override;
};