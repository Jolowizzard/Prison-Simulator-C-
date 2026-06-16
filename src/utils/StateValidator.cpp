#include "StateValidator.hpp"
#include <algorithm>

void StateValidator::validatePrisoner(PrisonerState& p){
    p.health = std::clamp(p.health, 0, 100);
    p.hunger = std::clamp(p.hunger, 0, 100);
    p.aggression = std::clamp(p.aggression, 0, 100);
    p.exhaustion = std::clamp(p.exhaustion, 0, 100);
}