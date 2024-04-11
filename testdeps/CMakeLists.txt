if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/Catch2/CMakeLists.txt)
  message(FATAL_ERROR "git submodules have not been set up yet. Run git submodule update --init")
endif()

add_subdirectory(Catch2)
