add_library(prisoner
    PrisonerProcess.cpp
)

target_include_directories(prisoner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(prisoner
    PUBLIC
        core
        rooms
        behavior
        fight
        utils
)
