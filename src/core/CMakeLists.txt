add_library(core
    SimulationCore.cpp
    PathFinder.cpp
    PrisonGraph.cpp
    CantineState.cpp
    SimulationProcess.cpp
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(core
    PUBLIC
        rooms
        fight
        utils
        behavior
        gui
)