cmake_minimum_required(VERSION 3.21)
project(tape-tests)

set(CMAKE_CXX_STANDARD 23)

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} tape-tests.cpp)

target_link_libraries(${PROJECT_NAME} GTest::gtest GTest::gtest_main tape-lib)