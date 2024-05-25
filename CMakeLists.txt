cmake_minimum_required(VERSION 3.28)
project(tape)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib)
add_subdirectory(utilities)
add_subdirectory(util)
if (TESTS)
    add_subdirectory(tests)
endif ()
