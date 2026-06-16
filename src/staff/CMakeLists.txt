add_library(staff
    GuardProcess.cpp
    CookProcess.cpp
    JanitorProcess.cpp
    DoctorProcess.cpp
)

target_include_directories(staff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(staff
    PUBLIC
        core
        rooms
        behavior
        fight

)
