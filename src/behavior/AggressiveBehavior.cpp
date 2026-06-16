#include "AggressiveBehavior.hpp"

void AggressiveBehavior::decide(PrisonerProcess& prisoner, SimulationState* state){
    int opponent = prisoner.chooseOpponent();
    FightManager::tryFight(state, prisoner.getId(), opponent);
}