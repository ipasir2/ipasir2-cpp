cmake_minimum_required(VERSION 3.16.3)
project(ipasir2-cpp)


# ipasir2cpp.h can be used with C++17, but has extra features when
# used with C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_library(ipasir2cpp INTERFACE)
target_include_directories(ipasir2cpp INTERFACE include)

add_subdirectory(testdeps)
add_subdirectory(tests)
