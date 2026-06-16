#ifndef PRISON_CONFIG_HPP
#define PRISON_CONFIG_HPP

#include <vector>
#include <string>
#include "RoomType.hpp"

// Struktura opisująca pojedynczy pokój w configu
struct RoomCfg {
    int id;
    RoomType type;
    int capacity;
};

// Struktura opisująca korytarz
struct CorridorCfg {
    int from;
    int to;
    int distance;
};

// Główna struktura konfiguracyjna
struct PrisonConfig {
    int numPrisoners;
    int numGuards;
    int numCooks;
    int numJanitors;
    int numDoctors;
    
    // Szpital, Pralnia itp. - przechowujemy ID, żeby wiedzieć gdzie wysyłać ludzi
    int hospitalRoomId;
    int laundryRoomId;
    int lobbyRoomId;

    std::vector<RoomCfg> rooms;
    std::vector<CorridorCfg> corridors;

    // Konstruktor z wartościami domyślnymi (żeby zmienne nie były śmieciami)
    PrisonConfig() : 
        numPrisoners(0), numGuards(0), numCooks(0), 
        numJanitors(0), numDoctors(0),
        hospitalRoomId(-1), laundryRoomId(-1), lobbyRoomId(-1) {}
};

#endif