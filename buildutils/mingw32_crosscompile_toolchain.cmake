# This toolchain file is for cross-compiling the examples and tests
# for Windows on a Linux host, with mingw32. Pass this file to cmake
# via -DCMAKE_TOOLCHAIN_FILE=<this-file>

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
add_compile_definitions(WIN32)
