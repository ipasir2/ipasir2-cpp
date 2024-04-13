if(WIN32)
  # Lingeling can't be built on Windows currently
  return()
endif()


add_executable(examples-all
  01_readme.cpp
  02_trivial_portfolio.cpp
  03_find_backbones.cpp
  04_custom_types.cpp
  example_utils.cpp
  example_utils.h
  main.cpp
)

target_compile_options(examples-all PRIVATE ${IPASIR2CPP_CXX_OPTS})
target_link_libraries(examples-all PRIVATE ipasir2cpp lingeling-static)


if (NOT WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # With clang, libatomic is not automatically linked on Linux
  target_link_libraries(examples-all PRIVATE atomic)
endif()

add_test(NAME examples-all COMMAND examples-all)


add_executable(example-utils-testsuite
  example_utils.cpp
  example_utils_tests.cpp
)

target_compile_options(example-utils-testsuite PRIVATE ${IPASIR2CPP_CXX_OPTS})
target_link_libraries(example-utils-testsuite PRIVATE Catch2::Catch2WithMain)

add_test(NAME example-utils-testsuite COMMAND example-utils-testsuite)
