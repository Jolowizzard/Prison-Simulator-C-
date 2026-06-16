#include "PrisonerProcess.hpp"
#include "RoomManager.hpp"
#include "FightManager.hpp"
#include "PathFinder.hpp"
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <StateValidator.hpp>
#include <CantineState.hpp>

PrisonerProcess::PrisonerProcess(int id, SimulationState* state)
    : id(id), SimulationProcess(state) {}

void PrisonerProcess::run() {
    srand(time(nullptr) ^ getpid()); // inicjalizacja generatora liczb pseudolosowych
    //std::cout << "[PRISONER " << id << "] start" << std::endl;

    while (isSimulationRunning() && state->prisoners[id].alive) {
        auto& me = state->prisoners[id];

        think();

        act();
        
        sleep(1);
    }
    RoomManager::leaveRoom(state,id);

    //std::cout << "[PRISONER " << id << "] end" << std::endl;
}

void PrisonerProcess::think() {
    auto& me = state->prisoners[id];

        // Jeżeli walczy, to nie rób nic innego ??
        if(me.state == PrisonerStateEnum::FIGHTING){
            return;
        }

    pthread_mutex_lock(&me.mutex);
    // Wzrost głodu
    me.hunger++;

    if (me.hunger > 100 ||  me.exhaustion > 100 || me.health <= 0){
        me.alive = false;
        pthread_mutex_unlock(&me.mutex);
        return;
    }
    // Wzrost agresji, jeżeli głodny
    if (me.hunger > 24 && me.hunger%2 == 0) {
        if(me.exhaustion > 60){
            me.aggression += 2;
        }else{
            me.aggression += 1;
        }
    }

    if (me.hunger % 2 == 0 ){
        me.exhaustion ++;
    }
    pthread_mutex_unlock(&me.mutex);
}

void PrisonerProcess::act() {
    auto& me = state->prisoners[id];

    if(!me.alive)
        return;

    if (me.state == PrisonerStateEnum::FIGHTING) {
        if (currentFightIndex != -1) {
            // Jeżeli agresor prowadź walkę
            FightManager::updateFight(state, currentFightIndex);
            
            // Sprawdzenie, czy walka się nie skończyła
            if (!state->fights[currentFightIndex].active) {
                currentFightIndex = -1; // Resetujemy lokalny wskaźnik
            }
        } 
        return;
    }

    if(me.state == PrisonerStateEnum::HEALING){
        return;
    }

    if (!currentPath.empty()) {
        move();
        return;
    }

    if(me.health < 60){
        goHospital();
        return;
    }

    if(me.hunger > 45){
        if(state->rooms[me.currentRoom].type != RoomType::CANTINE){
            goToRoom(RoomType::CANTINE);
            return;
        }
        else{
            goEat();
        }
    }

    if(me.exhaustion > 60){
        if(me.currentRoom != me.cellId){    
            goToCell();
            return;
        }
        else{
            goSleep();
        }
    }

    if (!currentPath.empty() || isTraversing) {
        me.state = PrisonerStateEnum::MOVING; // Ustaw stan dla GUI
        move();
        
        // Jeśli po ruchu ścieżka się skończyła, wróć do IDLE
        if (currentPath.empty() && !isTraversing) {
            me.state = PrisonerStateEnum::IDLE;
        }
        return;
    }

    if (me.aggression > 50) {
            int opponent = chooseOpponent();
            if (opponent != -1) {
                // Spróbuj rozpocząć walkę
                int fightId = FightManager::startFight(state, id, opponent);
                if (fightId != -1) {
                    currentFightIndex = fightId;
                    return;
                }
            }
        }

    if (me.state == PrisonerStateEnum::IDLE && state->rooms[me.currentRoom].type == RoomType::HOSPITAL) {
        goToCell();
        return;
    }

    if(me.currentRoom != me.cellId){
        if(rand() % 4 == 1){ // jedna czwarta szans na to, że więzień wróci do celi
            goToCell();
            return;
        }
    }

    if(state->rooms[me.currentRoom].type != RoomType::LOBBY){
        if(rand() % 2 == 1)
            goToRoom(RoomType::LOBBY);
    }
}

void PrisonerProcess::goToRoom(RoomType t){

    int targetRoomId = -1;

    for(int i = 0; i < state->roomsCount ; i++){
        if(state->rooms[i].type == t){
            targetRoomId = i;
            break; 
        }
    }

    if (targetRoomId == -1) return;

    PrisonerState& me = state->prisoners[id];

    std::vector<int> path = PathFinder::shortestPath(state->graph, me.currentRoom, targetRoomId);

    if (path.empty()) return;

    if (path[0] == me.currentRoom) {
        path.erase(path.begin());
    }

    currentPath = path;

    pthread_mutex_lock(&me.mutex);
    me.state = PrisonerStateEnum::MOVING;
    pthread_mutex_unlock(&me.mutex);
    
    //std::cout << "[PRISONER " << id << "] wyznaczyl trase do pokoju " << targetRoomId 
              //<< ". Dlugosc: " << currentPath.size() << std::endl;
}

void PrisonerProcess::move(){
    if (currentPath.empty()){
        return;
    }

    if (moveTimer > 0) {
        moveTimer--;
        return; 
    }

    // Pobierz nastepny krok
    int nextRoomId = currentPath.front();
    PrisonerState& me = state->prisoners[id];


    int travelCost = getTravelTime(me.currentRoom, nextRoomId);

    if (!isTraversing) {
        // Dopiero chcemy wejść na krawędź
        int cost = getTravelTime(me.currentRoom, nextRoomId);
        if (cost > 0) {
            moveTimer = cost;
            isTraversing = true;
            // Odejmujemy 1 od razu, bo ten cykl też trwa
            moveTimer--; 
            return; 
        }
    }

    if (isTraversing) {
        if (moveTimer > 0) {
            moveTimer--;
            return;
        }
        // Timer zszedł do 0 -> koniec podróży
        isTraversing = false;
    }
    
    // Wykonaj ruch fizyczny w symulacji 
    RoomManager::leaveRoom(state, id);
    RoomManager::enterRoom(state, id, nextRoomId);

    // Usun odwiedzony wezel ze sciezki
    currentPath.erase(currentPath.begin());

    //Zmiana stanu na idle - po dotarciu na miejsce
    if(currentPath.empty()){
        pthread_mutex_lock(&me.mutex);
        me.state = PrisonerStateEnum::IDLE;
        pthread_mutex_unlock(&me.mutex);
    }
}

int PrisonerProcess::chooseOpponent(){
    PrisonerState& me = state->prisoners[id];
    int roomId;

    pthread_mutex_lock(&me.mutex);
    roomId = me.currentRoom;
    pthread_mutex_unlock(&me.mutex);

    if (roomId < 0) return -1;
    
    RoomState& room = state->rooms[roomId];

    pthread_mutex_lock(&room.mutex);

    int opponent = -1;
    for (int i = 0; i < room.occupantCount; i++){
        PrisonerStateEnum pState = state->prisoners[room.occupants[i]].state;
        if(room.occupants[i] != id && pState != PrisonerStateEnum::EATING && pState != PrisonerStateEnum::SLEEPING && pState != PrisonerStateEnum::HEALING && pState != PrisonerStateEnum::QUEUEING){ // adding security for prisoners waiting in line
            opponent = room.occupants[i];
            break; 
        }
    }

    pthread_mutex_unlock(&room.mutex);
    return opponent;
}

void PrisonerProcess::goToCell(){
    PrisonerState& me = state->prisoners[id];

    std::vector<int> path = PathFinder::shortestPath(state->graph, me.currentRoom, me.cellId);

    if (path.empty()) return;

    if (path[0] == me.currentRoom) {
        path.erase(path.begin());
    }

    currentPath = path;
}

int PrisonerProcess::getId(){
    return id;
}

int PrisonerProcess::getTravelTime(int from, int to) {
    return PathFinder::edgeWeight(state->graph, from, to); 
}

void PrisonerProcess::goEat() {
    CantineState& c = state->cantine;
    PrisonerState& me = state->prisoners[id];

    // Sprawdź, czy fizycznie zmieścisz się w kolejce (limit osób w pokoju/kolejce)
    if (sem_trywait(&c.qeueLine) != 0)
        return;

    // Wejdź do kolejki i POBIERZ BILET (Ticket Lock)
    pthread_mutex_lock(&c.mutex);
    int myTicket = c.ticketDispenser++; // Pobierz obecny numer i zwiększ licznik dla następnego
    c.qeueSize++;
    pthread_mutex_unlock(&c.mutex);

    // Zaktualizuj stan na QUEUEING
    pthread_mutex_lock(&me.mutex);
    me.state = PrisonerStateEnum::QUEUEING;
    pthread_mutex_unlock(&me.mutex);

    // Czekaj na swoją kolej (Active Waiting z tickiem symulacji)
    bool haveEaten = false;
    
    while (isSimulationRunning()) {
        // Sprawdzamy czy to już nasza kolej
        pthread_mutex_lock(&c.mutex);
        bool isMyTurn = (c.nowServing == myTicket);
        pthread_mutex_unlock(&c.mutex);

        if (isMyTurn && me.alive) {
            if (sem_trywait(&c.mealsAvailable) == 0) {
                haveEaten = true;
                break;
            }
        }

        if(!me.alive){
            pthread_mutex_lock(&c.mutex);
            c.nowServing++;
            c.qeueSize--;
            pthread_mutex_unlock(&c.mutex);
            sem_post(&c.qeueLine);
            return;
        }
        think(); 
        usleep(1000000);
    }

    if (!haveEaten) {
        return;
    }
    sem_post(&c.qeueLine); 

    pthread_mutex_lock(&c.mutex);
    c.nowServing++;
    c.currentMeals--;
    c.qeueSize--;  
    pthread_mutex_unlock(&c.mutex);

    sem_post(&c.freeSlots);

    // Konsumpcja
    pthread_mutex_lock(&me.mutex);
    me.state = PrisonerStateEnum::EATING;
    me.hunger = std::max(0, me.hunger - 50); // Zabezpieczenie przed ujemnym głodem
    StateValidator::validatePrisoner(me);
    pthread_mutex_unlock(&me.mutex);

    int timeEating = (rand() % (MAX_EATING_TIME - MIN_EATING_TME)) + MIN_EATING_TME; 
    sleep(timeEating); 
}

void PrisonerProcess::wander() {
    // Wybierz losowy pokój inny niż obecny
    PrisonerState &me = state->prisoners[id];

    int target = rand() % state->roomsCount;
    while (target == me.currentRoom) {
        target = rand() % state->roomsCount;
    }
    
    // std::cout << "[GUARD " << id << "] Patroluje -> cel: " << target << "\n";
    //setDestination(target);
}

void PrisonerProcess::goSleep() {
    PrisonerState& me = state->prisoners[id];
    RoomState& cell = state->rooms[me.cellId];

    while (isSimulationRunning()) {
        // próbujemy zdobyć pościel
        if (sem_trywait(&cell.cleanBedding) == 0) {
            break; // sukces
        }

        // nadal "żyjemy"
        think();

        if(!me.alive)
            return;

        pthread_mutex_lock(&me.mutex);
        me.state = PrisonerStateEnum::IDLE; // czeka / wierci się
        pthread_mutex_unlock(&me.mutex);

        sleep(1); // tick symulacji
    }

    // śpimy
    pthread_mutex_lock(&me.mutex);
    me.state = PrisonerStateEnum::SLEEPING;
    pthread_mutex_unlock(&me.mutex);

    pthread_mutex_lock(&state->beddingStats.mutex);
    state->beddingStats.clean--;
    state->beddingStats.dirty++;
    pthread_mutex_unlock(&state->beddingStats.mutex);

    int sleepTime = (rand() % 5) + 3;
    sleep(sleepTime);

    pthread_mutex_lock(&me.mutex);
    me.exhaustion = std::max(0, me.exhaustion - 70);
    StateValidator::validatePrisoner(me);
    pthread_mutex_unlock(&me.mutex);

    sem_post(&cell.dirtyBedding);
}

void PrisonerProcess::goHospital() {
    HospitalState& h = state->hospital;
    PrisonerState& me = state->prisoners[id];

    if (state->rooms[me.currentRoom].type != RoomType::HOSPITAL) {
        goToRoom(RoomType::HOSPITAL);
        return;
    }

    if(!waitWithShutdown(&h.freeBeds))
        return;

    pthread_mutex_lock(&h.mutex);
    h.occupiedBeds++;
    pthread_mutex_unlock(&h.mutex);

    pthread_mutex_lock(&me.mutex);
    me.state = PrisonerStateEnum::HEALING;
    pthread_mutex_unlock(&me.mutex);
}
