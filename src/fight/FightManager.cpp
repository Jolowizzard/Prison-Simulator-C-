#include "FightManager.hpp"
#include "StateValidator.hpp"
#include <iostream>

bool FightManager::tryFight(SimulationState* state,
                            int attackerId,
                            int defenderId) {
    if (attackerId == defenderId) return false;

    PrisonerState& a = state->prisoners[attackerId];
    PrisonerState& d = state->prisoners[defenderId];

    PrisonerState* first  = (attackerId < defenderId) ? &a : &d;
    PrisonerState* second = (attackerId < defenderId) ? &d : &a;

    pthread_mutex_lock(&first->mutex);
    pthread_mutex_lock(&second->mutex);

    //Checking if either of fighters is alive
    if (!a.alive || !d.alive) {
        pthread_mutex_unlock(&second->mutex);
        pthread_mutex_unlock(&first->mutex);
        return false;
    }

    //Checking if either of fighters is already in fight
    if ((a.state == PrisonerStateEnum::FIGHTING) || (d.state == PrisonerStateEnum::FIGHTING)){
        pthread_mutex_unlock(&second->mutex);
        pthread_mutex_unlock(&first->mutex);
        return false;
    }

    d.state = PrisonerStateEnum::FIGHTING;
    a.state = PrisonerStateEnum::FIGHTING;

    //simple fight sequence for testing 
    d.health -= 10;
    a.health -= 3;

    //prisoners data validation
    StateValidator::validatePrisoner(d);
    StateValidator::validatePrisoner(a);
    
    //both fighters loose aggression
    d.aggression -= 4;
    a.aggression -= 4;


    std::cout << "[FIGHT] " << attackerId
              << " vs " << defenderId << std::endl;

    //log for testing
    std::cout << "Health- " << attackerId << " : " << a.health << ", "<< defenderId << " : " << d.health << std::endl; 


    pthread_mutex_unlock(&second->mutex);
    pthread_mutex_unlock(&first->mutex);

    return true;
}

int FightManager::startFight(SimulationState* state, int attackerId, int defenderId) {
        // Znajdź wolny slot na walkę
        for (int i = 0; i < MAX_ACTIVE_FIGHTS; i++) {
            FightState& fight = state->fights[i];
            pthread_mutex_lock(&fight.mutex);
            
            if (!fight.active) {
                // Inicjalizacja walki
                fight.active = true;
                fight.attackerId = attackerId;
                fight.defenderId = defenderId;
                fight.roomId = state->prisoners[attackerId].currentRoom;
                fight.durationLeft = 10; // Walka trwa 10 sekund (chyba że strażnik przerwie)
                
                // Ustawienie stanów więźniów
                state->prisoners[attackerId].state = PrisonerStateEnum::FIGHTING;
                state->prisoners[defenderId].state = PrisonerStateEnum::FIGHTING;
                
                pthread_mutex_unlock(&fight.mutex);
                return i; // Zwracamy ID walki
            }
            pthread_mutex_unlock(&fight.mutex);
        }
        return -1; // Brak miejsca na nową walkę
    }

void FightManager::updateFight(SimulationState* state, int fightIndex) {
    FightState& fight = state->fights[fightIndex];
    pthread_mutex_lock(&fight.mutex);

    if (!fight.active) {
        pthread_mutex_unlock(&fight.mutex);
        return;
    }

    fight.durationLeft--;
    //std::cout << "[FIGHT] Trwa walka w pokoju " << fight.roomId << ". Pozostalo: " << fight.durationLeft << "s\n";

    // Koniec czasu walki
    if (fight.durationLeft <= 0) {
        endFight(state, fight);
    }

    applyDamage(state, fight.attackerId, fight.defenderId);

    pthread_mutex_unlock(&fight.mutex);
}

// Funkcja dla Strażnika
void FightManager::breakFight(SimulationState* state, int fightIndex) {
    FightState& fight = state->fights[fightIndex];
    
    pthread_mutex_lock(&fight.mutex);
    
    if (fight.active) {
        // Logika końca walki
        fight.active = false;
        
        int id1 = fight.attackerId;
        int id2 = fight.defenderId;

        pthread_mutex_unlock(&fight.mutex);

        forceStopPrisoner(state, id1, 10);
        forceStopPrisoner(state, id2, 10);

        //std::cout << "[FIGHT] Walka " << fightIndex << " przerwana i wyczyszczona.\n";
    } else {
        pthread_mutex_unlock(&fight.mutex);
    }
}

void FightManager::forceStopPrisoner(SimulationState* state, int pId, int aggression) {
    PrisonerState& p = state->prisoners[pId];
    pthread_mutex_lock(&p.mutex);
    
    if (p.alive) {
        p.state = PrisonerStateEnum::IDLE;
        p.aggression -= aggression; 
        StateValidator::validatePrisoner(p);
    }
    
    pthread_mutex_unlock(&p.mutex);
}

void FightManager::endFight(SimulationState* state, FightState& fight) {
    fight.active = false;
    
    // Przywracamy więźniów do IDLE
    state->prisoners[fight.attackerId].state = PrisonerStateEnum::IDLE;
    state->prisoners[fight.attackerId].aggression = 0; // Reset agresji

    state->prisoners[fight.defenderId].state = PrisonerStateEnum::IDLE;
    
    //std::cout << "[FIGHT] Walka zakonczona.\n";
}

void FightManager::applyDamage(SimulationState* state, int attackerId, int defenderId) {
    PrisonerState& attacker = state->prisoners[attackerId];
    PrisonerState& defender = state->prisoners[defenderId];

    PrisonerState* first  = (attackerId < defenderId) ? &attacker : &defender;
    PrisonerState* second = (attackerId < defenderId) ? &defender : &attacker;

    pthread_mutex_lock(&first->mutex);
    pthread_mutex_lock(&second->mutex);

    if (attacker.alive && defender.alive) {
        defender.health -= 2;
        attacker.health -= 1;

        // Sprawdzamy czy ktoś nie umarł
        if (defender.health <= 0) {
            defender.alive = false;
            defender.health = 0;
            
        }
        if (attacker.health <= 0) {
            attacker.alive = false;
            attacker.health = 0;
        }
    }

    pthread_mutex_unlock(&second->mutex);
    pthread_mutex_unlock(&first->mutex);
}
