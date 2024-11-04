add_subdirectory(memory)

if (NOT TEST_MODE)
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/arch/${ARCH}/linker.ld")
    if ("${ARCH}" STREQUAL "x86_64")
        set(OBJCOPY_OUTPUT_FORMAT "elf64-x86-64")
        set(OBJCOPY_ARCH "i386:x86-64")
    elseif ("${ARCH}" STREQUAL "aarch64")
        set(OBJCOPY_OUTPUT_FORMAT "elf64-littleaarch64")
        set(OBJCOPY_ARCH "aarch64")
    else ()
        message(FATAL_ERROR "Unsupported architecture: ${ARCH}")
    endif ()
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/font.o
            COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_OUTPUT_FORMAT} -B ${OBJCOPY_ARCH} font.psf ${CMAKE_CURRENT_BINARY_DIR}/font.o
            DEPENDS font.psf
            COMMENT "Generating font.o from font.psf"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(kernel
            ${CMAKE_CURRENT_BINARY_DIR}/font.o
            memory/memalloc.cpp
            memory/memalloc.h
            memory/get-page.cpp
            memory/get-page.h
            utils/debug.h
            include/cstddef
            main.cpp
            include/stdint.h
            include/stddef.h
    )
    set_target_properties(kernel PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
    target_compile_options(kernel PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_CXX_FLAGS}>)
    target_link_options(kernel PRIVATE "-T" "${LINKER_SCRIPT}")
    target_link_options(kernel PRIVATE ${KERNEL_LINKER_OPTIONS})
    target_include_directories(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
