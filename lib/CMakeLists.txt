cmake_minimum_required(VERSION 3.28)
project(tape-lib)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE HEADERS include/*)
file(GLOB_RECURSE SOURCES src/*)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
