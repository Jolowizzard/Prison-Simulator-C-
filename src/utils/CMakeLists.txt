add_library(utils
    StateValidator.cpp
    ConfigLoader.cpp
    PrisonConfig.cpp
)

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(utils
    PUBLIC
        fight
        prisoner
        rooms
        core
)
