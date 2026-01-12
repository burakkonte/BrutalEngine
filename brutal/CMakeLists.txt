cmake_minimum_required(VERSION 3.20)
project(BrutalEngine LANGUAGES C CXX)

add_subdirectory(brutal)

if(MSVC)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT playground)
endif()
