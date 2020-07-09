cmake_minimum_required(VERSION 3.10)
project(RISC_V_simulator)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CSS_FLAGS}  -O2")
add_executable(RISC_V_simulator main.cpp riscv.hpp)