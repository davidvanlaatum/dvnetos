if (NOT TEST_MODE AND ARCH STREQUAL "aarch64")
    target_sources(kernel PRIVATE
            paging.cpp
            paging.h
    )
endif ()
