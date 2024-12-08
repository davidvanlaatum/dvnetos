if (TEST_MODE)
  include(configure-test)
  add_executable(memalloc_test)
  target_include_directories(
      memalloc_test
      PRIVATE
      ${CMAKE_SOURCE_DIR}/kernel
  )
  target_compile_definitions(
      memalloc_test
      PRIVATE
      DEBUG
  )
  configure_test(memalloc_test)
endif ()
cus_target_sources(kernel PRIVATE memalloc.cpp memalloc.h get-page.cpp get-page.h MemMap.cpp MemMap.h)
cus_target_sources(memalloc_test
    memalloc.cpp
    memalloc.h
    memalloc_test.cpp
)
