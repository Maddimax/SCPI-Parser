cmake_minimum_required(VERSION 3.10)

project(SCPI VERSION 0.1.1 LANGUAGES CXX)

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

add_executable(scpi-test SCPI.cpp) 
