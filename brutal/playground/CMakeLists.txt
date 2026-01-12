cmake_minimum_required(VERSION 3.20)

add_executable(playground src/main.cpp)

target_link_libraries(playground PRIVATE brutal_engine)

if(WIN32)
    # Link Windows subsystem for console app
    target_link_options(playground PRIVATE /SUBSYSTEM:CONSOLE)
endif()
