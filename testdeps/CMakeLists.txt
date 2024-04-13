if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/Catch2/CMakeLists.txt
   OR NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/ipasir2_solver_lingeling/CMakeLists.txt)
  message(FATAL_ERROR "git submodules have not been set up yet. Run git submodule update --init")
endif()

add_subdirectory(Catch2)

if (NOT WIN32)
  set(LGL_BUILD_TOOLS OFF)
  add_subdirectory(ipasir2_solver_lingeling)
endif()
