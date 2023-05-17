cmake_minimum_required(VERSION 3.22)
project(OS)

set(CMAKE_CXX_STANDARD 14)

include_directories(hw1)

add_executable(OS
        hw1/Commands.cpp
        hw1/Commands.h
        hw1/signals.cpp
        hw1/signals.h
        hw1/smash.cpp)
