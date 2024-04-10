if(WIN32)
  # Currently the minisat dependency can't be built in cross-compilation
  return()
endif()


include(${CMAKE_CURRENT_LIST_DIR}/../testdeps/ipasir2/buildutils/load_solvers.cmake)

load_minisat()

add_executable(examples-all
  01_readme.cpp
  02_trivial_portfolio.cpp
  03_find_backbones.cpp
  04_custom_types.cpp
  example_utils.cpp
  example_utils.h
  main.cpp
)

target_link_libraries(examples-all PRIVATE ipasir2cpp minisat)
target_compile_options(examples-all PRIVATE ${IPASIR2_CXX_OPTS})


if (NOT WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # With clang, libatomic is not automatically linked on Linux
  target_link_libraries(examples-all PRIVATE atomic)
endif()

add_test(NAME examples-all COMMAND examples-all)


add_executable(example-utils-testsuite
  example_utils.cpp
  example_utils_tests.cpp
)

target_link_libraries(example-utils-testsuite PRIVATE doctest)
target_compile_options(example-utils-testsuite PRIVATE ${IPASIR2_CXX_OPTS})

add_test(NAME example-utils-testsuite COMMAND example-utils-testsuite)
