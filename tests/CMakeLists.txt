cmake_minimum_required(VERSION 3.21)
project(tape-tests)

set(CMAKE_CXX_STANDARD 23)

find_package(GTest REQUIRED)

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} GTest::gtest GTest::gtest_main tape-lib)