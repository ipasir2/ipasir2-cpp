# ipasir2-cpp - C++ wrapper for IPASIR-2 solvers
#
# In standalone builds, this project contains the ipasir2-cpp tests and examples.
# Building it is not required for using ipasir2cpp.h in other software.

cmake_minimum_required(VERSION 3.19)

if (DEFINED PROJECT_NAME)
  # ipasir2-cpp has been added via add_subdirectory(), so just define the ipasir2cpp target:
  add_library(ipasir2cpp INTERFACE)
  target_include_directories(ipasir2cpp INTERFACE include deps/ipasir2)
  return()
endif()


project(ipasir2-cpp-tests)

option(WITH_DOXYGEN "Add a target `doxygen` building Doxygen files for ipasir2cpp.h" OFF)

include(buildutils/doxygen.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)

add_library(ipasir2cpp INTERFACE
  include/ipasir2cpp.h
  include/ipasir2cpp_dl.h
)

target_include_directories(ipasir2cpp INTERFACE include)
target_link_libraries(ipasir2cpp INTERFACE ipasir2)


# Only basic compiler options that are used for all targets are set here. More special
# options, for instance for sanitizers, are set on a per-target basis. This project
# is deliberately light on compiler options to make sure that the headers don't force
# clients to set any.
if (MSVC)
  set(IPASIR2CPP_CXX_OPTS
    /DNOMINMAX    # Keep Windows.h from #defining min() and max()
    /permissive-  # Be strict about C++ conformance
    /W4           # Use highest warning level

    # TODO: fix size types in ipasir2.h, use size_t instead of int32_t
    # The following two warnings are related to possible loss of data when implicitly casting ints
    # to narrower types. This happens for instance when std::vector<int32>::size() is used with
    # ipasir2_add(). Disabling them for now:
    /wd4267
    /wd4244
  )
else()
  # Assume GCC or clang:
  set(IPASIR2CPP_CXX_OPTS
    -Wall
    -Wextra
    -Wunused
    -Warray-bounds
    -pedantic
  )
endif()

# In this project, ctest executes all testsuites as well as the example code
enable_testing()

add_subdirectory(deps)
add_subdirectory(testdeps)

add_subdirectory(tests)
add_subdirectory(examples)
