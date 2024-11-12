cmake_minimum_required(VERSION 3.22)
project(server C)

set(CMAKE_C_STANDARD 11)

add_executable(server src/main.c src/server.c)
target_include_directories(server PUBLIC include)
