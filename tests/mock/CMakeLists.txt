# The IPASIR2 mock is built as a shared library so it can be used to test
# the dynamic-loading functions of the IPASIR2 wrapper:
add_library(ipasir2mock SHARED ipasir2_mock.cpp ipasir2_mock.h)
target_compile_options(ipasir2mock PRIVATE ${IPASIR2CPP_CXX_OPTS})
target_link_libraries(ipasir2mock PRIVATE ipasir2)
target_compile_definitions(ipasir2mock PRIVATE BUILDING_IPASIR_SHARED_LIB)
target_compile_definitions(ipasir2mock INTERFACE IPASIR_SHARED_LIB)

# Remove the "lib" prefix from the shared library filename to simplify
# crosscompiling, since the file name is referenced in code. For instance,
# mingw32 adds the "lib" prefix by default.
set_target_properties(ipasir2mock PROPERTIES PREFIX "")
