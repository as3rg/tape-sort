cmake_minimum_required(VERSION 3.28)
project(utilities)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE HEADERS include/*.h)
file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
