cmake_minimum_required(VERSION 3.28)
project(tape-lib)

set(CMAKE_CXX_STANDARD 23)

file(GLOB HEADERS include/*.h)
file(GLOB SOURCES src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
