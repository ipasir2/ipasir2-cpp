set(sources
  mock/ipasir2_mock_tests.cpp
  add_clause_tests.cpp
  detail_tests.cpp
  query_lit_tests.cpp
  solve_tests.cpp
  test_main.cpp
  tests.cpp
)


# The IPASIR2 mock is built as a shared library so it can be used to test
# the dynamic-loading functions of the IPASIR2 wrapper: 
add_library(ipasir2mock SHARED mock/ipasir2_mock.cpp)
target_link_libraries(ipasir2mock PRIVATE ipasir2 doctest)


function(add_test_executable NAME CXX_STANDARD)
  add_executable(${NAME} ${sources})
  target_compile_options(${NAME} PRIVATE -Wall -Wextra -pedantic)
  target_link_libraries(${NAME} PRIVATE ipasir2 ipasir2cpp doctest ipasir2mock)
  target_include_directories(${NAME} PRIVATE .)
  set_target_properties(${NAME} PROPERTIES CXX_STANDARD ${CXX_STANDARD})

  add_executable(${NAME}-dlsym dlsym_tests.cpp)
  target_compile_options(${NAME}-dlsym PRIVATE -Wall -Wextra -pedantic)
  target_link_libraries(${NAME}-dlsym PRIVATE ipasir2 ipasir2cpp doctest)
  target_include_directories(${NAME}-dlsym PRIVATE .)
  set_target_properties(${NAME}-dlsym PROPERTIES CXX_STANDARD ${CXX_STANDARD})
endfunction()


add_test_executable(ipasir2-cpp-tests-cxx17 17)
add_test_executable(ipasir2-cpp-tests-cxx20 20)
