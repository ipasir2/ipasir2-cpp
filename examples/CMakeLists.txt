include(${CMAKE_CURRENT_LIST_DIR}/../testdeps/ipasir2/buildutils/load_solvers.cmake)

load_minisat()

add_executable(examples_all
  01_readme.cpp
  02_trivial_portfolio.cpp
  example_utils.cpp
  main.cpp
)

target_link_libraries(examples_all PRIVATE ipasir2cpp minisat)


if (NOT WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # With clang, libatomic is not automatically linked on Linux
  target_link_libraries(examples_all PRIVATE atomic)
endif()


add_executable(example_utils_tests
  example_utils.cpp
  example_utils_tests.cpp
)

target_link_libraries(example_utils_tests PRIVATE doctest)
