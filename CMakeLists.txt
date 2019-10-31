cmake_minimum_required(VERSION 3.15)
project(calculator C)

set(CMAKE_C_STANDARD 99)

add_executable(calculator main.c)
target_link_libraries(calculator m)