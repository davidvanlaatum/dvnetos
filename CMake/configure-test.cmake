function(configure_test TARGET)
  target_link_libraries(${TARGET} PRIVATE GTest::gtest_main)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
      target_link_libraries(${TARGET} PRIVATE stdc++ m)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      target_link_libraries(${TARGET} PRIVATE c++)
    else ()
      message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
    endif ()
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${TARGET} PRIVATE stdc++)
  else ()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
  endif ()
  target_link_options(${TARGET} PRIVATE ${COVERAGE_LINKER_OPTIONS})
  target_compile_options(${TARGET} PRIVATE ${COVERAGE_CXX_FLAGS})
  gtest_discover_tests(${TARGET})
endfunction()
