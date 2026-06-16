#include "GuardProcess.hpp"
#include "PathFinder.hpp"
#include "RoomManager.hpp"
#include "FightManager.hpp"
#include <iostream>
#include <unistd.h>
#include <cstdlib> // do rand()
#include <cstdio>  // do sprintf

GuardProcess::GuardProcess(int id, SimulationState* state)
    : id(id), SimulationProcess(state), currentRoom(3) {
    
    // Rejestracja początkowa w RoomManager (ID ujemne dla strażników)
    RoomManager::enterRoom(state, -1 * (id + 1), currentRoom); 
}

void GuardProcess::run() {
    srand(time(nullptr) ^ getpid());
    // std::cout << "[GUARD " << id << "] Rozpoczyna wachte." << std::endl;

    while (isSimulationRunning()) {
        think();
        move();
        sleep(1);
    }
}

void GuardProcess::think() {
    // Sprawdź czy są aktywne walki
    int activeFightIndex = scanForFights();

    if (activeFightIndex != -1) {
        FightState& fight = state->fights[activeFightIndex];
        
        if (!fight.active) return; 

        if (currentRoom == fight.roomId) {
            if (!isTraversing) {
                sprintf(state->guards[id].actionInfo, "INTERWENCJA!");
                FightManager::breakFight(state, activeFightIndex);
                currentPath.clear();
                
                pthread_mutex_lock(&state->guards[id].mutex);
                state->guards[id].isMoving = false;
                pthread_mutex_unlock(&state->guards[id].mutex);
            }
            return;
        } 
        
        if (!currentPath.empty() && currentPath.back() == fight.roomId) {
            sprintf(state->guards[id].actionInfo, "BIEGNE DO POKOJU %d", fight.roomId);
            return; 
        }

        sprintf(state->guards[id].actionInfo, "ALARM W POKOJU %d!", fight.roomId);
        setDestination(fight.roomId);
        return;
    }

    if (currentPath.empty() && !isTraversing) {
        wander();
    }
}

void GuardProcess::move() {
    if (currentPath.empty()) {
        return;
    }

    if (moveTimer > 0) {
        moveTimer--;
        return; 
    }

    int nextRoomId = currentPath.front();
    
    // Strażnicy mają ujemne ID w RoomManagerze: -1, -2
    int guardEntityId = -1 * (id + 1);

    if (!isTraversing) {
        int cost = getTravelTime(currentRoom, nextRoomId);
        if (cost > 0) {
            moveTimer = cost;
            isTraversing = true;
            moveTimer--; // Odejmujemy 1 od razu
            return; 
        }
    }

    if (isTraversing) {
        if (moveTimer > 0) {
            moveTimer--;
            return;
        }
        isTraversing = false;
    }
    
    RoomManager::leaveRoom(state, guardEntityId);

    RoomManager::enterRoom(state, guardEntityId, nextRoomId);

    currentRoom = nextRoomId;
    currentPath.erase(currentPath.begin());

    pthread_mutex_lock(&state->guards[id].mutex);
    state->guards[id].currentRoom = currentRoom;
    
    if (currentPath.empty()) {
        state->guards[id].isMoving = false;
    } else {
        state->guards[id].isMoving = true;
    }
    pthread_mutex_unlock(&state->guards[id].mutex);

    // std::cout << "[GUARD " << id << "] przeszedl do pokoju " << currentRoom << std::endl;
}

int GuardProcess::scanForFights() {
    for (int i = 0; i < MAX_ACTIVE_FIGHTS; i++) {
        if (state->fights[i].active) {
            return i;
        }
    }
    return -1;
}

void GuardProcess::setDestination(int targetRoomId) {
    if (targetRoomId == currentRoom) return;

    std::vector<int> path = PathFinder::shortestPath(state->graph, currentRoom, targetRoomId);

    if (path.empty()) return;

    if (path[0] == currentRoom) {
        path.erase(path.begin());
    }

    currentPath = path;
    isTraversing = false; 
    moveTimer = 0;

    // Ustawiamy flagę ruchu w pamięci współdzielonej
    pthread_mutex_lock(&state->guards[id].mutex);
    state->guards[id].isMoving = true;
    pthread_mutex_unlock(&state->guards[id].mutex);
}

void GuardProcess::wander() {
    int target = rand() % state->roomsCount;
    while (target == currentRoom) {
        target = rand() % state->roomsCount;
    }
    
    sprintf(state->guards[id].actionInfo, "Patroluje %d!", target);
    setDestination(target);
}

int GuardProcess::getTravelTime(int from, int to) {
    return PathFinder::edgeWeight(state->graph, from, to);
}