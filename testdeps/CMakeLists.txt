if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/doctest/doctest/doctest.h
   OR NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/ipasir2/ipasir2.h)
  message(FATAL_ERROR "git submodules have not been set up yet. Run git submodule update --init")
endif()


add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE doctest/doctest)

add_library(ipasir2 INTERFACE)
target_include_directories(ipasir2 INTERFACE ipasir2)

