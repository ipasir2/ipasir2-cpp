# The IPASIR2 mock is built as a shared library so it can be used to test
# the dynamic-loading functions of the IPASIR2 wrapper:
add_library(ipasir2mock SHARED ipasir2_mock.cpp)
target_link_libraries(ipasir2mock PRIVATE ipasir2)
