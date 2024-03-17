add_executable(ipasir2-cpp-tests
    mock/ipasir2_mock.cpp
    mock/ipasir2_mock_tests.cpp

    test_main.cpp
)

target_compile_options(ipasir2-cpp-tests PRIVATE -Wall -Wextra -pedantic)
target_link_libraries(ipasir2-cpp-tests PRIVATE ipasir2 doctest)
