add_library(fight
    FightManager.cpp
)

target_include_directories(fight PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(fight
    PUBLIC
        prisoner
)