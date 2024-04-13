set(test_base_dir ${CMAKE_CURRENT_LIST_DIR})

# add_test_executables(<NAME> [source1] [source2] ...)
#
# Adds targets <NAME>-cpp17 (compiled with C++17) and <NAME>-cpp20 (compiled
# with C++20). This is done because the wrapper has special cases for C++20
# which should not break C++17 builds.
function(add_test_executables BASENAME)
  foreach(cxx_standard IN ITEMS 17 20)
    set(name ${BASENAME}-cpp${cxx_standard})

    add_executable(${name} ${ARGN})
    set_target_properties(${name} PROPERTIES CXX_STANDARD ${cxx_standard})

    if (NOT WIN32)
      set(sanitizers "-fsanitize=address,undefined")
      target_compile_options(${name} PRIVATE ${sanitizers})
      target_link_options(${name} PRIVATE ${sanitizers})
    endif()

    target_include_directories(${name} PRIVATE ${test_base_dir})
    target_compile_options(${name} PRIVATE ${IPASIR2CPP_CXX_OPTS})
    target_link_libraries(${name} PRIVATE ipasir2cpp Catch2::Catch2WithMain)

    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  endforeach()
endfunction()

add_subdirectory(dlsym)
add_subdirectory(main)
add_subdirectory(mock)
