#include "SimulationGUI.hpp"
#include <iostream>
#include <unistd.h> // dla usleep
#include <string>   // dla std::string
#include <cmath>
#include <vector>

SimulationGUI::SimulationGUI(SimulationState* state) : state(state), isRunning(true), mapDirty(true) {
    initNcurses();
    createWindows();
}

SimulationGUI::~SimulationGUI() {
    destroyWindows();
    cleanupNcurses();
}

void SimulationGUI::createWindows() {
    int h, w;
    getmaxyx(stdscr, h, w);

    int dashW = w * 0.6;
    int mapW  = w - dashW;

    dashboardWin = newwin(h, dashW, 0, 0);
    mapWin       = newwin(h, mapW,  0, dashW);

    box(dashboardWin, 0, 0);
    box(mapWin, 0, 0);

    mapDirty = true;
}

void SimulationGUI::destroyWindows() {
    if (dashboardWin) delwin(dashboardWin);
    if (mapWin) delwin(mapWin);
    dashboardWin = nullptr;
    mapWin = nullptr;
}

void SimulationGUI::initNcurses() {
    initscr();              // Start ncurses
    cbreak();               // Wyłącz buforowanie linii
    noecho();               // Nie wyświetlaj wpisywanych znaków
    curs_set(0);            // Ukryj kursor
    nodelay(stdscr, TRUE);  // getch() nie będzie blokować programu
    start_color();          // Włącz kolory
    keypad(stdscr, TRUE);

    // Definicja par kolorów (Tekst, Tło)
    // 1: IDLE (Biały)
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    // 2: MOVING (Żółty)
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    // 3: FIGHTING (Czerwony)
    init_pair(3, COLOR_RED, COLOR_BLACK);
    // 4: EATING (Zielony)
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    // 5: SLEEPING (Niebieski/Cyjan)
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
    
    // --- NOWE: Kolor dla strażnika (Magenta/Fiolet) ---
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);

    init_pair(7, COLOR_BLUE, COLOR_BLACK); // woźny

    init_pair(8, COLOR_YELLOW, COLOR_BLACK);
    
    // 9: HEALING (Zielony jasny / medyczny)
    init_pair(9, COLOR_GREEN, COLOR_BLACK);

    init_pair(20, COLOR_WHITE,  COLOR_BLACK); // CELL
    init_pair(21, COLOR_YELLOW, COLOR_BLACK); // CANTINE
    init_pair(22, COLOR_GREEN,  COLOR_BLACK); // HOSPITAL
    init_pair(23, COLOR_CYAN,   COLOR_BLACK); // LAUNDRY
    init_pair(24, COLOR_RED,    COLOR_BLACK); // ISOLATION

}

void SimulationGUI::cleanupNcurses() {
    endwin(); // Przywróć terminal
}

void SimulationGUI::run() {
    while (isRunning) {
        handleInput();
        werase(dashboardWin); 
        drawDashboard();

        //if (mapDirty) {
            werase(mapWin);
            drawPrisonMap();   // PEŁNE rysowanie
            //mapDirty = false;
        //}

        wnoutrefresh(dashboardWin);
        wnoutrefresh(mapWin);
        doupdate();

        usleep(100000); // 10 FPS
    }
}

void SimulationGUI::drawDashboard() {
    // Rysowanie ramki
    box(dashboardWin, 0, 0);

    // Tytuł
    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, 0, 2, " PRISON SIMULATION MONITOR (Press 'q' to quit) ");
    wattroff(dashboardWin, A_BOLD);

    int row = 2;

    // --- BEDDING STATUS ---
    pthread_mutex_lock(&state->beddingStats.mutex);
    int clean = state->beddingStats.clean;
    int dirty = state->beddingStats.dirty;
    int total = state->beddingStats.totalChanges;
    pthread_mutex_unlock(&state->beddingStats.mutex);

    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row, 2,
        "BEDDING STATUS | Clean: %d | Dirty: %d | Changed total: %d",
        clean, dirty, total
    );
    wattroff(dashboardWin, A_BOLD);

    row += 2;

    // --- HOSPITAL STATUS ---
    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row, 2, "HOSPITAL STATUS");
    wattroff(dashboardWin, A_BOLD);

    pthread_mutex_lock(&state->hospital.mutex);
    int totalBeds = state->hospital.totalBeds;
    int occupiedBeds = state->hospital.occupiedBeds;
    pthread_mutex_unlock(&state->hospital.mutex);

    mvwprintw(dashboardWin, row + 1, 2,
            "Beds: %d free / %d occupied",
            totalBeds - occupiedBeds,
            occupiedBeds);

    mvwprintw(dashboardWin, row + 2, 2,
            "Doctors on duty: %d",
            state->doctorsCount);

    row += 4;

    // --- PRISONERS SECTION ---
    wattron(dashboardWin, A_BOLD);
    // Przesuwam nagłówek o 1 w prawo, żeby nie wchodził na ramkę
    mvwprintw(dashboardWin, row, 1, "%-5s | %-10s | %-15s | %-8s | %-8s | %-8s | %-8s", 
              "ID", "ROOM ID", "STATE", "HUNGER", "AGGR", "HP", "EXHAUST");
    mvwprintw(dashboardWin, row + 1, 1, "----------------------------------------------------------------------");
    wattroff(dashboardWin, A_BOLD);

    row += 2;

    for (int i = 0; i < state->prisonersCount; i++) {
        PrisonerState& p = state->prisoners[i];

        if (pthread_mutex_trylock(&p.mutex) != 0) {
             mvwprintw(dashboardWin, row, 1, "%-5d | [SYNCING]  | ............... | ........ | ........ | ........ | ........", p.id);
             row++;
             continue; // Przechodzimy do następnego więźnia bez czekania
        }

        if (!p.alive) {
            pthread_mutex_unlock(&p.mutex);
            mvwprintw(dashboardWin, row, 1, "%-5d | ---------- | --------------- | -------- | -------- | -------- | --------", i);
            row++;
            continue; 
        }

        int id = p.id;
        int roomId = p.currentRoom;
        int hunger = p.hunger;
        int aggr = p.aggression;
        int hp = p.health;
        int exh = p.exhaustion;
        PrisonerStateEnum s = p.state;
        pthread_mutex_unlock(&p.mutex);

        // Dobór koloru
        int colorPair = stateToColorPair(s);
        std::string stateStr = stateToString(s);

        // Rysowanie wiersza
        wattron(dashboardWin, COLOR_PAIR(colorPair));
        if (s == PrisonerStateEnum::FIGHTING) wattron(dashboardWin, A_BOLD);

        // Uwaga: x=1 (margines lewy)
        mvwprintw(dashboardWin, row, 1, "%-5d | %-10d | %-15s | %-8d | %-8d | %-8d | %-8d", 
                 id, roomId, stateStr.c_str(), hunger, aggr, hp, exh);

        wattroff(dashboardWin, A_BOLD);
        wattroff(dashboardWin, COLOR_PAIR(colorPair));
        
        // Nadpisanie koloru pokoju
        RoomState& room = state->rooms[roomId];
        int roomColor = roomTypeToColor(room.type);

        wattron(dashboardWin, COLOR_PAIR(roomColor));
        // Oryginalnie było 8, tutaj dajemy 9 bo zaczęliśmy od x=1
        mvwprintw(dashboardWin, row, 9, "%-10d", roomId);  
        wattroff(dashboardWin, COLOR_PAIR(roomColor));

        row++;
    }

    // --- GUARD MONITOR ---
    row += 2; 

    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row, 2, "GUARD MONITOR");
    mvwprintw(dashboardWin, row + 1, 1, "%-5s | %-10s | %-20s", "ID", "ROOM ID", "ACTION");
    mvwprintw(dashboardWin, row + 2, 1, "-------------------------------------------");
    wattroff(dashboardWin, A_BOLD);

    row += 3;

    for (int i = 0; i < state->guardsCount; i++) {
        GuardState& g = state->guards[i];

        pthread_mutex_lock(&g.mutex);
        int id = g.id;
        int roomId = g.currentRoom;
        std::string action = g.actionInfo; 
        pthread_mutex_unlock(&g.mutex);

        // Kolor 6 (Magenta)
        wattron(dashboardWin, COLOR_PAIR(6) | A_BOLD);
        mvwprintw(dashboardWin, row, 1, "G%-4d | %-10d | %-20s", id, roomId, action.c_str());
        wattroff(dashboardWin, COLOR_PAIR(6) | A_BOLD);

        // Nadpisanie koloru pokoju
        RoomState& room = state->rooms[roomId];
        int roomColor = roomTypeToColor(room.type);

        wattron(dashboardWin, COLOR_PAIR(roomColor));
        // Oryginalnie było 8, tutaj dajemy 9 bo zaczęliśmy od x=1
        mvwprintw(dashboardWin, row, 9, "%-10d", roomId);  
        wattroff(dashboardWin, COLOR_PAIR(roomColor));

        row++;
    }

    // --- CANTINE STATUS ---
    row += 2;

    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row, 2, "CANTINE STATUS");
    mvwprintw(dashboardWin, row + 1, 1, "-------------------------------------------");
    wattroff(dashboardWin, A_BOLD);

    row += 2;

    pthread_mutex_lock(&state->cantine.mutex);
    int meals = state->cantine.currentMeals;
    int queueCount = state->cantine.qeueSize;
    pthread_mutex_unlock(&state->cantine.mutex);

    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row,     2, "Available meals : %d", meals);
    mvwprintw(dashboardWin, row + 1, 2, "Prisoners in queue : %d / %d", queueCount, MAX_QUEUE);
    wattroff(dashboardWin, A_BOLD);

    // --- JANITOR MONITOR ---
    row += 3;

    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row, 2, "JANITOR MONITOR");
    mvwprintw(dashboardWin, row + 1, 1, "%-5s | %-10s | %-25s", "ID", "ROOM ID", "ACTION");
    mvwprintw(dashboardWin, row + 2, 1, "-----------------------------------------------------");
    wattroff(dashboardWin, A_BOLD);

    row += 3;

    for (int i = 0; i < state->janitorsCount; i++) {
        JanitorState& j = state->janitors[i];

        pthread_mutex_lock(&j.mutex);
        int id = j.id;
        int roomId = j.currentRoom;
        std::string action = j.actionInfo;
        pthread_mutex_unlock(&j.mutex);

        wattron(dashboardWin, COLOR_PAIR(7) | A_BOLD);
        mvwprintw(dashboardWin, row, 1, "J%-4d | %-10d | %-25s", id, roomId, action.c_str());
        wattroff(dashboardWin, COLOR_PAIR(7) | A_BOLD);

        // Nadpisanie koloru pokoju
        RoomState& room = state->rooms[roomId];
        int roomColor = roomTypeToColor(room.type);

        wattron(dashboardWin, COLOR_PAIR(roomColor));
        // Oryginalnie było 8, tutaj dajemy 9 bo zaczęliśmy od x=1
        mvwprintw(dashboardWin, row, 9, "%-10d", roomId);  
        wattroff(dashboardWin, COLOR_PAIR(roomColor));


        row++;
    }

    // --- DOCTOR MONITOR ---
    row += 2;
    
    wattron(dashboardWin, A_BOLD);
    mvwprintw(dashboardWin, row, 2, "DOCTOR MONITOR");
    mvwprintw(dashboardWin, row + 1, 1, "%-5s | %-10s | %-25s", "ID", "ROOM ID", "ACTION");
    mvwprintw(dashboardWin, row + 2, 1, "-----------------------------------------------------");
    wattroff(dashboardWin, A_BOLD);

    row += 3;

    for (int i = 0; i < state->doctorsCount; i++) {
        DoctorState& d = state->doctors[i];

        pthread_mutex_lock(&d.mutex);
        int id = d.id;
        int roomId = d.currentRoom;
        std::string action = d.actionInfo;
        pthread_mutex_unlock(&d.mutex);

        // Używam 9 (Medyczny/Zielony) zdefiniowany w initNcurses
        wattron(dashboardWin, COLOR_PAIR(9) | A_BOLD);
        mvwprintw(dashboardWin, row, 1,
                "D%-4d | %-10d | %-25s",
                id, roomId, action.c_str());
        wattroff(dashboardWin, COLOR_PAIR(9) | A_BOLD);

        row++;
    }
}

void SimulationGUI::handleInput() {
    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
        isRunning = false;
    }
    else if (ch == KEY_RESIZE) {
        handleResize();
    }
}

void SimulationGUI::handleResize() {

    int h, w;
    getmaxyx(stdscr, h, w);

    // usuń stare okno mapy
    if (mapWin) {
        delwin(mapWin);
        mapWin = nullptr;
    }

    int mapStartX = w * 0.6;
    mapWin = newwin(h, w - mapStartX, 0, mapStartX);

    // WYMUSZ pełny redraw mapy
    mapDirty = true;
}

std::string SimulationGUI::stateToString(PrisonerStateEnum s) {
    switch (s) {
        case PrisonerStateEnum::IDLE:     return "IDLE";
        case PrisonerStateEnum::MOVING:   return "MOVING >>";
        case PrisonerStateEnum::FIGHTING: return "!! FIGHTING !!";
        case PrisonerStateEnum::EATING:   return "EATING";
        case PrisonerStateEnum::SLEEPING: return "SLEEPING";
        case PrisonerStateEnum::QUEUEING: return "QUEUEING";
        case PrisonerStateEnum::HEALING:  return "HEALING";
        default:                          return "UNKNOWN";
    }
}

int SimulationGUI::stateToColorPair(PrisonerStateEnum s) {
    switch (s) {
        case PrisonerStateEnum::IDLE:     return 1;
        case PrisonerStateEnum::MOVING:   return 2;
        case PrisonerStateEnum::FIGHTING: return 3;
        case PrisonerStateEnum::EATING:   return 4;
        case PrisonerStateEnum::SLEEPING: return 5;
        case PrisonerStateEnum::QUEUEING: return 8;
        case PrisonerStateEnum::HEALING:  return 9;
        default:                          return 1;
    }
}

int SimulationGUI::roomTypeToColor(RoomType t) {
    switch (t) {
        case RoomType::CELL:      return 20;
        case RoomType::CANTINE:   return 21;
        case RoomType::HOSPITAL:  return 22;
        case RoomType::LOUNDRY:   return 23;
        case RoomType::LOBBY: return 24;
        default:                  return 1;
    }
}

void SimulationGUI::drawPrisonMap() {
    int winH, winW;
    getmaxyx(mapWin, winH, winW);

    // Czyszczenie i ramka
    werase(mapWin);
    box(mapWin, 0, 0);
    wattron(mapWin, A_BOLD);
    mvwprintw(mapWin, 0, 2, " PRISON MAP ");
    wattroff(mapWin, A_BOLD);

    int roomCount = state->roomsCount;
    if (roomCount == 0) {
        wrefresh(mapWin);
        return;
    }

    // =========================
    // 1. KONFIGURACJA SIATKI
    // =========================
    // Obliczamy ile kolumn i wierszy potrzebujemy, żeby zmieścić pokoje
    int cols = std::ceil(std::sqrt(roomCount));
    // Jeśli wyjdzie bardzo płasko, wymuszamy więcej kolumn dla panoramy
    if (cols < 2 && roomCount > 1) cols = 2; 
    
    int rows = std::ceil((double)roomCount / cols);

    // Marginesy wewnętrzne (żeby nie rysować po ramce okna)
    int marginX = 2;
    int marginY = 2;
    
    // Dostępna przestrzeń robocza
    int usableW = winW - (2 * marginX);
    int usableH = winH - (2 * marginY);

    // Rozmiar pojedynczej komórki w siatce
    int cellW = usableW / cols;
    int cellH = usableH / rows;

    // Stały rozmiar wizualny pokoju (pudełka)
    const int ROOM_BOX_W = 9; // szerokość "pudełka" pokoju
    const int ROOM_BOX_H = 5; // wysokość "pudełka" pokoju

    // =========================
    // 2. OBLICZANIE WSPÓŁRZĘDNYCH ŚRODKÓW
    // =========================
    std::vector<std::pair<int,int>> centers(roomCount);

    for (int i = 0; i < roomCount; i++) {
        int r = i / cols;
        int c = i % cols;

        // Oblicz środek komórki siatki
        // margin + (numer_kolumny * szerokość_komórki) + połowa_szerokości
        int cx = marginX + (c * cellW) + (cellW / 2);
        int cy = marginY + (r * cellH) + (cellH / 2);

        centers[i] = {cx, cy};
    }

    // =========================
    // 3. RYSOWANIE POŁĄCZEŃ (Warstwa spodnia)
    // =========================
    // Rysujemy linie najpierw, żeby pokoje przykryły je w miejscach styku
    wattron(mapWin, A_DIM); // Linie lekko przygaszone

    for (int i = 0; i < roomCount; i++) {
        for (int j = i + 1; j < roomCount; j++) {
            // Jeśli istnieje połączenie w grafie
            if (state->graph.adjacency[i][j] != NO_EDGE) {
                drawLine(
                    mapWin,
                    centers[i].first, centers[i].second,
                    centers[j].first, centers[j].second
                );
            }
        }
    }
    wattroff(mapWin, A_DIM);

    // =========================
    // 4. RYSOWANIE POKOI (Warstwa wierzchnia)
    // =========================
    for (int i = 0; i < roomCount; i++) {
        // Pobieramy wyliczony wcześniej środek
        int cx = centers[i].first;
        int cy = centers[i].second;

        // Obliczamy lewy górny róg "pudełka" tak, aby cx, cy było w środku
        int x = cx - (ROOM_BOX_W / 2);
        int y = cy - (ROOM_BOX_H / 2);

        RoomState& room = state->rooms[i];
        int color = roomTypeToColor(room.type);

        // Ustawiamy kolor pokoju
        wattron(mapWin, COLOR_PAIR(color) | A_BOLD);

        // Rysowanie pudełka 
        // Używamy mvwhline/mvwvline dla ładniejszych linii, ale printw też zadziała
        mvwprintw(mapWin, y,     x, "+-------+");
        mvwprintw(mapWin, y + 1, x, "| ID:%-3d|", i);
        
        // Skrót nazwy pokoju (np. CEL, CAN, HOS)
        std::string name = roomTypeToString(room.type);
        if(name.length() > 5) name = name.substr(0, 5);
        mvwprintw(mapWin, y + 2, x, "| %-5s |", name.c_str());
        
        // Wyświetlenie liczby ludzi w pokoju (opcjonalne, ale przydatne)
        // int count = 0; // Tutaj musiałbyś zliczyć więźniów w pokoju
        mvwprintw(mapWin, y + 3, x, "| C: %-3d|", room.capacity); // Pusty wiersz lub info
        mvwprintw(mapWin, y + 4, x, "| P: %-3d|", room.occupantCount); 

        mvwprintw(mapWin, y + 5, x, "+-------+");

        wattroff(mapWin, COLOR_PAIR(color) | A_BOLD);
    }
}



std::string SimulationGUI::roomTypeToString(RoomType t) {
    switch (t) {
        case RoomType::CELL: return "CELL";
        case RoomType::CANTINE: return "CANTINE";
        case RoomType::HOSPITAL: return "HOSPITAL";
        case RoomType::LOUNDRY: return "LAUNDRY";
        case RoomType::LOBBY: return "LOBBY";
        //case RoomType::ISOLATION: return "ISO";
        default: return "ROOM";
    }
}

std::pair<int,int> SimulationGUI::roomCenter(
    int roomIndex,
    int startX,
    int cols,
    int cellW,
    int cellH
) {
    int r = roomIndex / cols;
    int c = roomIndex % cols;

    int x = startX + c * cellW + cellW / 2;
    int y = 2 + r * cellH + cellH / 2;

    return {x, y};
}

void SimulationGUI::drawLine(WINDOW* win, int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        // Rysujemy kropkę jako korytarz. 
        // Sprawdzamy, czy nie wyjeżdżamy poza okno.
        if (x1 > 0 && y1 > 0) {
            // Używamy '.' dla estetyki, można zmienić na ACS_CKBOARD
            mvwaddch(win, y1, x1, '.'); 
        }

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}


