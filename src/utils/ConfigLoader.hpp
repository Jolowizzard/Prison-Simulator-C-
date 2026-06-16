#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include "PrisonConfig.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> // dla transform

class ConfigLoader {
public:
    // Metoda ładująca config z pliku
    static PrisonConfig loadFromFile(const std::string& path) {
        PrisonConfig config;
        std::ifstream file(path);

        if (!file.is_open()) {
            std::cerr << "[CONFIG] Nie mozna otworzyc pliku: " << path << ". Ladowanie domyslnych.\n";
            return loadDefaults();
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue; // Pomiń puste i komentarze

            std::stringstream ss(line);
            std::string key;
            ss >> key;

            if (key == "PRISONERS") ss >> config.numPrisoners;
            else if (key == "GUARDS") ss >> config.numGuards;
            else if (key == "COOKS") ss >> config.numCooks;
            else if (key == "JANITORS") ss >> config.numJanitors;
            else if (key == "DOCTORS") ss >> config.numDoctors;
            else if (key == "ROOM") {
                // Format: ROOM <ID> <TYPE_STRING> <CAPACITY>
                int id, cap;
                std::string typeStr;
                ss >> id >> typeStr >> cap;
                RoomType type = stringToRoomType(typeStr);
                
                config.rooms.push_back({id, type, cap});

                // Automatyczne wykrywanie specjalnych pokoi
                if (type == RoomType::HOSPITAL) config.hospitalRoomId = id;
                if (type == RoomType::LOUNDRY) config.laundryRoomId = id;
                if (type == RoomType::LOBBY) config.lobbyRoomId = id;
            }
            else if (key == "LINK") {
                // Format: LINK <FROM> <TO> <DIST>
                int from, to, dist;
                ss >> from >> to >> dist;
                config.corridors.push_back({from, to, dist});
            }
        }
        
        std::cout << "[CONFIG] Zaladowano konfiguracje z pliku: " << path << "\n";
        return config;
    }

    // Metoda z Twoim dotychczasowym hardcoded setupem
    static PrisonConfig loadDefaults() {
        PrisonConfig config;
        config.numPrisoners = 4;
        config.numGuards = 1;
        config.numCooks = 1;
        config.numJanitors = 1;
        config.numDoctors = 2;

        // Pokoje (bazując na Twoim kodzie)
        config.rooms.push_back({0, RoomType::CANTINE, 10});
        config.rooms.push_back({1, RoomType::CELL, 2}); // CELL_CAPACITY zakladam ze to 2
        config.rooms.push_back({2, RoomType::CELL, 2});
        config.rooms.push_back({3, RoomType::LOBBY, 5});
        config.rooms.push_back({4, RoomType::LOUNDRY, 2});
        config.rooms.push_back({5, RoomType::HOSPITAL, 5});

        config.lobbyRoomId = 3;
        config.laundryRoomId = 4;
        config.hospitalRoomId = 5;

        // Korytarze
        config.corridors.push_back({0, 1, 3});
        config.corridors.push_back({0, 2, 3});
        config.corridors.push_back({0, 3, 2});
        config.corridors.push_back({1, 2, 2});
        config.corridors.push_back({4, 3, 2});
        config.corridors.push_back({3, 5, 3});

        std::cout << "[CONFIG] Zaladowano konfiguracje DOMYSLNA.\n";
        return config;
    }

private:
    static RoomType stringToRoomType(std::string s) {
        // Zamiana na uppercase dla pewności
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);

        if (s == "CANTINE") return RoomType::CANTINE;
        if (s == "CELL") return RoomType::CELL;
        if (s == "HOSPITAL") return RoomType::HOSPITAL;
        if (s == "LAUNDRY" || s == "LOUNDRY") return RoomType::LOUNDRY;
        if (s == "LOBBY") return RoomType::LOBBY;
        //if (s == "ISOLATION") return RoomType::ISOLATION;
        return RoomType::CELL; // Default
    }
};

#endif