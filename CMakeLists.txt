cmake_minimum_required(VERSION 3.15)
project(smc_fan_util)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-O3 -framework IOKit")

include_directories(.)

add_executable(smc_fan_util
        smc.h
        smc_fan_util.cpp
        smc.cpp)
