#pragma once
#include "SimulationCore.hpp"

class FightManager {
public:
    static bool tryFight(SimulationState* state, int attackerId, int defenderId);
    static int startFight(SimulationState* state, int attackerId, int defenderId);
    static void updateFight(SimulationState* state, int fightIndex);
    static void breakFight(SimulationState* state, int fightIndex);

private:
    static void endFight(SimulationState* state, FightState& fight);
    static void applyDamage(SimulationState* state, int attackerId, int defenderId);
    static void forceStopPrisoner(SimulationState* state, int prisonerId, int aggresionAfterBreak);
};
