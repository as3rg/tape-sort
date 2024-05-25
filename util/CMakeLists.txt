cmake_minimum_required(VERSION 3.21)
project(tape-util)

set(CMAKE_CXX_STANDARD 23)

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC tape-lib utilities)