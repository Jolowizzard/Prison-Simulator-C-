#pragma once
#include "SimulationCore.hpp"
#include <ncurses.h>
#include <string>

class SimulationGUI {
public:
    SimulationGUI(SimulationState* state);
    ~SimulationGUI();

    // Główna pętla GUI (uruchom to w osobnym wątku lub na końcu main)
    void run();

private:
    SimulationState* state;
    bool isRunning;
    bool mapDirty;

    WINDOW* dashboardWin;
    WINDOW* mapWin;

    // Metody pomocnicze
    void initNcurses();
    void cleanupNcurses();
    void drawDashboard();
    void handleInput();
    void handleResize();
    void createWindows();
    void destroyWindows();
    
    // Konwersje dla wyświetlania
    std::string stateToString(PrisonerStateEnum s);
    int stateToColorPair(PrisonerStateEnum s);
    int roomTypeToColor(RoomType t);
    void drawPrisonMap();
    std::string roomTypeToString(RoomType t);
    std::pair<int,int> roomCenter(int roomIndex, int startX, int cols, int cellW, int cellH); 
    void drawLine(WINDOW* win, int x1, int y1, int x2, int y2);
};