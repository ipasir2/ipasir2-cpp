set(test_base_dir ${CMAKE_CURRENT_LIST_DIR})

function(add_test_executables BASENAME)
  foreach(cxx_standard IN ITEMS 17 20)
    set(name ${BASENAME}-cpp${cxx_standard})
    add_executable(${name} ${ARGN})

    set(sanitizers, "-fsanitize=address,undefined")
    target_compile_options(${name} PRIVATE -Wall -Wextra -pedantic ${sanitizers})
    target_link_options(${name} PRIVATE ${sanitizers})
    target_link_libraries(${name} PRIVATE ipasir2cpp doctest)
    target_include_directories(${name} PRIVATE ${test_base_dir})
    set_target_properties(${name} PROPERTIES CXX_STANDARD ${cxx_standard})

    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  endforeach()
endfunction()

add_subdirectory(dlsym)
add_subdirectory(main)
add_subdirectory(mock)
