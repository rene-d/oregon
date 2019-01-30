# rene-d 01/2019

cmake_minimum_required(VERSION 3.7)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_compile_options(-Wall -Wextra)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

add_executable(analyze analyze.cpp)
target_include_directories(analyze PRIVATE include)

add_executable(analyze_arduino analyze.cpp)
target_compile_definitions(analyze_arduino PRIVATE ARDUINO)
target_include_directories(analyze_arduino PRIVATE include)
