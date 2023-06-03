cmake_minimum_required(VERSION 3.22)
project(OS C)

set(CMAKE_C_STANDARD 99)

add_executable(OS
        hw3/server.c hw3/segel.c hw3/request.h hw3/request.c)
