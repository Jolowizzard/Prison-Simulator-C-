#pragma once
#include "SimulationCore.hpp"

class RoomManager {
public:
    static bool enterRoom(SimulationState* state, int prisonerId, int roomId);
    static void leaveRoom(SimulationState* state, int prisonerId);
};
