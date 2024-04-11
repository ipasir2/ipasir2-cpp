if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/ipasir2/ipasir2.h)
  message(FATAL_ERROR "git submodules have not been set up yet. Run git submodule update --init")
endif()

add_library(ipasir2 INTERFACE)
target_include_directories(ipasir2 INTERFACE ipasir2)
