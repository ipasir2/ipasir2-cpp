set(sources
  add_clause_tests.cpp
  custom_types.h
  detail_tests.cpp
  ipasir2_mock_factory.h
  ipasir2_mock_tests.cpp
  options_tests.cpp
  query_lit_tests.cpp
  set_export_tests.cpp
  set_terminate_tests.cpp
  solve_tests.cpp
  tests.cpp
)

add_test_executables(main-testsuite ${sources})
target_link_libraries(main-testsuite-cpp17 PRIVATE ipasir2mock)
target_link_libraries(main-testsuite-cpp20 PRIVATE ipasir2mock)
