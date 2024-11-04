if (TEST_MODE)
    add_executable(
            memalloc_test
            memalloc.cpp
            memalloc.h
            memalloc_test.cpp
    )
    target_link_libraries(
            memalloc_test
            GTest::gtest_main
            c++
    )
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
    set_target_properties(
            memalloc_test
            PROPERTIES
            CXX_COMPILER /usr/bin/clang++
    )
    gtest_discover_tests(memalloc_test)
endif ()
