set(sources
  mock/ipasir2_mock.cpp
  mock/ipasir2_mock_tests.cpp

  add_clause_tests.cpp
  detail_tests.cpp
  query_lit_tests.cpp
  solve_tests.cpp
  test_main.cpp
  tests.cpp
)


function(add_test_executable NAME CXX_STANDARD)
  add_executable(${NAME} ${sources})
  target_compile_options(${NAME} PRIVATE -Wall -Wextra -pedantic)
  target_link_libraries(${NAME} PRIVATE ipasir2 ipasir2cpp doctest)
  target_include_directories(${NAME} PRIVATE .)
  set_target_properties(${NAME} PROPERTIES CXX_STANDARD ${CXX_STANDARD})
endfunction()


add_test_executable(ipasir2-cpp-tests-cxx17 17)
add_test_executable(ipasir2-cpp-tests-cxx20 20)
