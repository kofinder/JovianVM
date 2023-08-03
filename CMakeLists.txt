cmake_minimum_required(VERSION 3.16.3)

project(
    JovianVM
    VERSION 1.0.0
    LANGUAGES C CXX
)

add_library(
    Library src/*
)

add_executable(Executable main.cpp)

target_link_libraries(Executable PUBLIC Library)