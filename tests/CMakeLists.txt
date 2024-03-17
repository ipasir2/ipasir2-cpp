add_executable(ipasir2-cpp-tests
    mock/ipasir2_mock.cpp
    mock/ipasir2_mock_tests.cpp

    test_main.cpp
    tests.cpp
)

target_compile_options(ipasir2-cpp-tests PRIVATE -Wall -Wextra -pedantic)
target_link_libraries(ipasir2-cpp-tests PRIVATE ipasir2 ipasir2cpp doctest)
target_include_directories(ipasir2-cpp-tests PRIVATE .)
