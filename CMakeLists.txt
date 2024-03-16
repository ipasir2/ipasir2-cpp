cmake_minimum_required(VERSION 3.16.3)
project(ipasir2-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(testdeps)
add_subdirectory(tests)

