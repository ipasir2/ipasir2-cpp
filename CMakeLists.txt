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
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_library(ipasir2cpp INTERFACE
  include/ipasir2cpp.h
  include/ipasir2cpp_dl.h
)

target_include_directories(ipasir2cpp INTERFACE include)
target_link_libraries(ipasir2cpp INTERFACE ipasir2)


# Only basic compiler options that are used for all targets are set here. More special
# options, for instance for sanitizers, are set on a per-target basis.
if (MSVC)
  add_compile_options(
    # Keep Windows.h from #define'ing min() and max():
    /DNOMINMAX

    # Make MSVC properly define __cplusplus. MSVC defaults to 199711L for compatibility.
    # TODO: when compiling with MSVC, check _MSVC_LANG instead, so this flag is not required
    /Zc:__cplusplus

    /permissive-
    /W4

    # TODO: fix size types in ipasir2.h, use size_t instead of int32_t
    # The following two warnings are related to possible loss of data when implicitly casting ints
    # to narrower types. This happens for instance when std::vector<int32>::size() is used with
    # ipasir2_add(). Disabling them for now:
    /wd4267
    /wd4244
  )
else()
  # Assume GCC or clang:
  add_compile_options(
    -Wall
    -Wextra
    -Wunused
    -pedantic
  )
endif()

# In this project, ctest executes all testsuites as well as the example code
enable_testing()

add_subdirectory(testdeps)
add_subdirectory(tests)
add_subdirectory(examples)
