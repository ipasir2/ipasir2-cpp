# This project contains the ipasir2-cpp tests and examples. Building it is not
# required for using ipasir2cpp.h in other software.

cmake_minimum_required(VERSION 3.19)
project(ipasir2-cpp-tests)


option(WITH_DOXYGEN "Add a target `doxygen` building Doxygen files for ipasir2cpp.h" OFF)

include(buildutils/doxygen.cmake)

# ipasir2cpp.h can be used with C++17, but has extra features when used with C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_library(ipasir2cpp INTERFACE
  include/ipasir2cpp.h
  include/ipasir2cpp_dl.h
)

target_include_directories(ipasir2cpp INTERFACE include)
target_link_libraries(ipasir2cpp INTERFACE ipasir2)


# In this project, ctest executes all testsuites as well as the example code
enable_testing()

add_subdirectory(testdeps)
add_subdirectory(tests)
add_subdirectory(examples)
