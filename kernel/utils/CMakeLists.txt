if (TEST_MODE)
  include(configure-test)
  add_executable(stdio_test stdio_test.cpp stdio.cpp)
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
if (TARGET kernel)
  target_sources(kernel PRIVATE
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
endif ()
