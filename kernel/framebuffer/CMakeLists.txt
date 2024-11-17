include(bin2h)
bin2h(
    SOURCE_FILE font.psf
    VARIABLE_NAME font_data
    HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/font_data.h
)

if (TEST_MODE)
  include(configure-test)
  add_executable(framebuffer_test
      framebuffer_test.cpp
      Framebuffer.h
      Framebuffer.cpp
      VirtualConsole.cpp
      VirtualConsole.h
  )
  target_include_directories(
      framebuffer_test
      PRIVATE
      ${CMAKE_SOURCE_DIR}/kernel
      ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_compile_definitions(
      framebuffer_test
      PRIVATE
      DEBUG
      kvsnprintf=vsnprintf
      ksnprintf=snprintf
  )
  set_target_properties(
      framebuffer_test
      PROPERTIES
      CXX_COMPILER /usr/bin/clang++
  )
  configure_test(framebuffer_test)
else ()
  target_sources(kernel PRIVATE
      Framebuffer.h
      Framebuffer.cpp
      VirtualConsole.cpp
      VirtualConsole.h
  )
  target_include_directories(kernel PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif ()
