if (NOT TEST_MODE AND ARCH STREQUAL "x86_64")
    target_sources(kernel PRIVATE
            paging.cpp
            paging.h
    )
endif ()
