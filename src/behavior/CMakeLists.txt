add_library(behavior
    BehaviorStrategy.cpp
)

target_include_directories(behavior PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(behavior
    PUBLIC
        prisoner
)