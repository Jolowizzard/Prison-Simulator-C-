add_library(rooms
    RoomState.cpp
    RoomType.cpp
    RoomManager.cpp
)

target_include_directories(rooms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rooms
    PUBLIC
        core
)
