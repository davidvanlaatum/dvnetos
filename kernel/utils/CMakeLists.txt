if (TEST_MODE)
  include(configure-test)
  add_executable(stdio_test)
  target_include_directories(
      stdio_test
      PRIVATE
      ${CMAKE_SOURCE_DIR}/kernel
  )
  target_compile_definitions(
      stdio_test
      PRIVATE
      DEBUG
  )
  configure_test(stdio_test)
endif ()
cus_target_sources(stdio_test stdio_test.cpp stdio.cpp)
cus_target_sources(kernel
    bytes.h
    bytes.cpp
    cstring.cpp
    debug.h
    inttostring.cpp
    inttostring.h
    panic.cpp
    panic.h
    stdio.cpp
)
