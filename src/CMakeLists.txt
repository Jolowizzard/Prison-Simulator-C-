add_subdirectory(core)
add_subdirectory(prisoner)
#Commented until further use
add_subdirectory(rooms)
add_subdirectory(fight)
add_subdirectory(behavior)
add_subdirectory(staff)
#add_subdirectory(ipc)
add_subdirectory(gui)
add_subdirectory(utils)

add_executable(prison_sim 
    main.cpp
    core/SimulationCore.cpp
    prisoner/PrisonerProcess.cpp
)

target_link_libraries(prison_sim
    core
    prisoner
    fight
    rooms
    behavior
    staff
    #ipc
    gui
    utils
    pthread
    rt
    ncurses
)
