add_library(gui
    SimulationGUI.cpp
)

target_include_directories(gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gui
    PUBLIC
        core
)