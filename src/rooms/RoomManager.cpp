#include "RoomManager.hpp"

bool RoomManager::enterRoom(SimulationState* state, int prisonerId, int roomId) {
    RoomState& room = state->rooms[roomId];
    PrisonerState& prisoner = state->prisoners[prisonerId];

    pthread_mutex_lock(&room.mutex);
    pthread_mutex_lock(&prisoner.mutex);

    if (room.occupantCount >= room.capacity) {
        pthread_mutex_unlock(&prisoner.mutex);
        pthread_mutex_unlock(&room.mutex);
        return false;
    }

    room.occupants[room.occupantCount++] = prisonerId;
    prisoner.currentRoom = roomId;

    pthread_mutex_unlock(&prisoner.mutex);
    pthread_mutex_unlock(&room.mutex);

    return true;
}

void RoomManager::leaveRoom(SimulationState* state, int prisonerId) {
    PrisonerState& prisoner = state->prisoners[prisonerId];
    int roomId = prisoner.currentRoom;
    if (roomId < 0) return;

    RoomState& room = state->rooms[roomId];

    pthread_mutex_lock(&room.mutex);
    pthread_mutex_lock(&prisoner.mutex);

    for (int i = 0; i < room.occupantCount; ++i) {
        if (room.occupants[i] == prisonerId) {
            room.occupants[i] = room.occupants[--room.occupantCount];
            break;
        }
    }

    prisoner.currentRoom = -1;

    pthread_mutex_unlock(&prisoner.mutex);
    pthread_mutex_unlock(&room.mutex);
}
