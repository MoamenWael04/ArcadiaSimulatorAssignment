cmake_minimum_required(VERSION 4.0)
project(ArcadiaSimulatorAssignment)

set(CMAKE_CXX_STANDARD 20)

add_executable(ArcadiaSimulatorAssignment main.cpp
        ArcadiaEngine.h
        ArcadiaEngine.cpp)
