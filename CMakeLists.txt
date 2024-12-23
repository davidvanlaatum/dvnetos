cmake_minimum_required(VERSION 3.29)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE INTERNAL "" FORCE)
include(CMakePrintHelpers)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include(arch-alias)
include(utils)
find_package(Clang)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER ${CLANG})
set(CMAKE_CXX_COMPILER ${CLANGPP})
set(CMAKE_ASM_COMPILER ${CLANG})
set(CMAKE_LINKER_TYPE LLD)
set(CMAKE_CXX_USING_LINKER_LLD "-fuse-ld=lld")
set(CMAKE_CXX_USING_LINKER_MODE FLAG)
set(CMAKE_C_USING_LINKER_LLD "-fuse-ld=lld")
set(CMAKE_C_USING_LINKER_MODE FLAG)
set(CMAKE_C_LINKER_PREFERENCE)
if (CLANG_SCAN_DEPS)
  set(CMAKE_C_COMPILER_CLANG_SCAN_DEPS ${CLANG_SCAN_DEPS})
  set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS ${CLANG_SCAN_DEPS})
endif ()
set(TEST_MODE OFF CACHE BOOL "Enable test mode")
set(ARCH "" CACHE STRING "Target architecture (x86_64 or aarch64)")
if (TEST_MODE)
  set(ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
else ()
  if (NOT ARCH)
    message(FATAL_ERROR "ARCH is not defined. Please specify -DARCH=x86_64 or -DARCH=aarch64 when running CMake.")
  endif ()
endif ()
handleArchAlias(ARCH)
if (NOT TEST_MODE)
  if (ARCH STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
  elseif (ARCH STREQUAL "aarch64")
    set(CMAKE_SYSTEM_PROCESSOR arm64)
  else ()
    message(FATAL_ERROR "ARCH must be x86_64 or aarch64 NOT '${ARCH}'")
  endif ()
  configure_file(CMake/toolchain.cmake.in toolchain.cmake @ONLY)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/toolchain.cmake)
endif ()
project(dvnetos VERSION 0.1.0 LANGUAGES C CXX ASM)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
add_link_options(-v)
enable_testing()
if (TEST_MODE)
  find_program(VALGRIND "valgrind")
  if (VALGRIND)
    set(MEMORYCHECK_COMMAND ${VALGRIND})
    set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1")
  endif ()
endif ()
include(CTest)
if (TEST_MODE)
  set(INSTALL_GTEST OFF)
  include(FetchContent)
  FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
      EXCLUDE_FROM_ALL
      FIND_PACKAGE_ARGS NAMES GTest
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  include(GoogleTest)
endif ()
if (CLANG_TIDY)
  set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY} --use-color)
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} --use-color)
endif ()
if (NOT TEST_MODE)
  add_compile_options_if_supported(CXX -nostdlib KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -nostartfiles KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -nostdinc KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -nostdinc++ KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -ffreestanding KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-sse KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-sse2 KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-sse3 KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-ssse3 KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-sse4.1 KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-sse4.2 KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-avx KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-avx2 KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-mmx KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mno-3dnow KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -mcmodel=kernel KERNEL_CXX_FLAGS)
  add_compile_options_if_supported(CXX -fPIE KERNEL_CXX_FLAGS)

  add_linker_options_if_supported(CXX -nostdlib KERNEL_LINKER_OPTIONS)
  add_linker_options_if_supported(CXX -nodefaultlibs KERNEL_LINKER_OPTIONS)
  add_linker_options_if_supported(CXX -ffreestanding KERNEL_LINKER_OPTIONS)
  add_linker_options_if_supported(CXX -Wl,-Map=output.map KERNEL_LINKER_OPTIONS)
  add_linker_options_if_supported(CXX -mcmodel=kernel KERNEL_LINKER_OPTIONS)
  add_linker_options_if_supported(CXX -Werror KERNEL_LINKER_OPTIONS)
endif ()
add_compile_options_if_supported(CXX -Wall LOCAL_CXX_FLAGS)
add_compile_options_if_supported(CXX -pedantic LOCAL_CXX_FLAGS)
add_compile_options_if_supported(CXX -pipe LOCAL_CXX_FLAGS)
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  add_compile_options_if_supported(CXX -O0 LOCAL_CXX_FLAGS)
else ()
  add_compile_options_if_supported(CXX -O3 LOCAL_CXX_FLAGS)
endif ()
add_compile_options_if_supported(CXX -Werror LOCAL_CXX_FLAGS)
string(JOIN " " CMAKE_CXX_FLAGS ${LOCAL_CXX_FLAGS})

add_compile_definitions(CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

if (TEST_MODE)
  add_linker_options_if_supported(CXX -fprofile-instr-generate COVERAGE_LINKER_OPTIONS)
  add_linker_options_if_supported(CXX -fcoverage-mapping COVERAGE_LINKER_OPTIONS)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_LINK_OPTIONS ${COVERAGE_LINKER_OPTIONS})
  add_compile_options_if_supported(CXX -fprofile-instr-generate COVERAGE_CXX_FLAGS)
  add_compile_options_if_supported(CXX -fcoverage-mapping COVERAGE_CXX_FLAGS)
  cmake_pop_check_state()
else ()
  find_program(MAKE_EXE NAMES gmake make REQUIRED)
  include(ExternalProject)
  ExternalProject_Add(
      limine
      URL https://github.com/limine-bootloader/limine/archive/refs/heads/v8.x-binary.zip
      #      GIT_REPOSITORY https://github.com/limine-bootloader/limine.git
      #      GIT_TAG v8.x-binary
      #      GIT_PROGRESS TRUE
      #      GIT_SHALLOW TRUE
      BUILD_COMMAND ${MAKE_EXE}
      CONFIGURE_COMMAND ""
      STEP_TARGETS download update
      BUILD_IN_SOURCE TRUE
      INSTALL_COMMAND ${MAKE_EXE} install PREFIX=<INSTALL_DIR>
      USES_TERMINAL_DOWNLOAD TRUE
      USES_TERMINAL_UPDATE TRUE
      EXCLUDE_FROM_ALL TRUE
      DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/.cache
  )
  ExternalProject_Get_Property(limine INSTALL_DIR)
  set(LIMINE_PATH ${INSTALL_DIR})
  add_library(limine-bootloader INTERFACE)
  target_include_directories(limine-bootloader INTERFACE ${LIMINE_PATH}/include)
  add_dependencies(limine-bootloader limine)
endif ()

add_subdirectory(tools)

set(CMAKE_CXX_COMPILER_LAUNCHER ${WARNING_COLLECTOR_COMMAND} compile)
set(CMAKE_C_COMPILER_LAUNCHER ${WARNING_COLLECTOR_COMMAND} compile)
set(CMAKE_ASM_COMPILER_LAUNCHER ${WARNING_COLLECTOR_COMMAND} compile)
set(CMAKE_LINKER_LAUNCHER ${WARNING_COLLECTOR_COMMAND} link)
set(CMAKE_CXX_LINKER_LAUNCHER ${WARNING_COLLECTOR_COMMAND} link)
set(CMAKE_C_LINKER_LAUNCHER ${WARNING_COLLECTOR_COMMAND} link)
if (CMAKE_C_CLANG_TIDY)
  set(CMAKE_C_CLANG_TIDY ${WARNING_COLLECTOR_COMMAND} clang-tidy ${CMAKE_C_CLANG_TIDY})
  set(CMAKE_CXX_CLANG_TIDY ${WARNING_COLLECTOR_COMMAND} clang-tidy ${CMAKE_CXX_CLANG_TIDY})
endif ()

add_subdirectory(kernel)
add_subdirectory(image)

cmake_print_variables(CMAKE_SYSTEM_NAME CMAKE_SYSTEM_PROCESSOR CMAKE_OBJCOPY CMAKE_C_COMPILER CMAKE_CXX_COMPILER CMAKE_ASM_COMPILER CMAKE_LINKER CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_ASM_FLAGS CMAKE_LINKER_FLAGS KERNEL_CXX_FLAGS KERNEL_LINKER_OPTIONS)
getAllTargets(all_targets)
cmake_print_properties(TARGETS ${all_targets} PROPERTIES TYPE SOURCES SOURCE_DIR INCLUDE_DIRECTORIES LINK_DEPENDS LINK_OPTIONS COMPILE_OPTIONS LINKER_TYPE LINKER_LANGUAGE)
write_warning_files()

if (TEST_MODE)
  getAllTests(TEST_EXECUTABLES)
  set(COVERAGE_INPUT_FILES "")
  set(COVERAGE_EXECUTABLES "")
  foreach (TEST_EXECUTABLE ${TEST_EXECUTABLES})
    get_target_property(COVERAGE_FILES ${TEST_EXECUTABLE} COVERAGE_FILES)
    if (COVERAGE_FILES)
      file(RELATIVE_PATH REL ${CMAKE_CURRENT_BINARY_DIR} ${COVERAGE_FILES})
      list(APPEND COVERAGE_INPUT_FILES ${REL})
      list(APPEND COVERAGE_EXECUTABLES -object $<TARGET_FILE:${TEST_EXECUTABLE}>)
    endif ()
  endforeach ()
  if (LLVM_PROFDATA AND LLVM_COV)
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake CONTENT "set(CTEST_CUSTOM_PRE_TEST \"cmake -P clean-coverage-files.cmake\")")
    configure_file(CMake/clean-coverage-files.cmake.in clean-coverage-files.cmake @ONLY)
    add_custom_target(coverage
        COMMAND ${LLVM_PROFDATA} merge -sparse ${COVERAGE_INPUT_FILES} -o default.profdata
        COMMAND ${LLVM_COV} show -instr-profile=default.profdata ${COVERAGE_EXECUTABLES} --format=html -output-dir=coverage/
        COMMAND ${LLVM_COV} report -instr-profile=default.profdata ${COVERAGE_EXECUTABLES} --ignore-filename-regex='.*_test.cpp'
        COMMAND ${LLVM_COV} export -instr-profile=default.profdata ${COVERAGE_EXECUTABLES} --ignore-filename-regex='.*_test.cpp' --format=lcov > coverage.info
        COMMAND ${LCOV_COBERTURA_PATH} coverage.info --base-dir ${CMAKE_SOURCE_DIR} --output coverage.xml --demangle
        USES_TERMINAL
        DEPENDS venv
    )
    set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "coverage;coverage.info;default.profdata;coverage.xml")
  endif ()
endif ()
include(commit-hook)
write_commit_hook()
if (CLANG_FORMAT)
  get_all_sources(SOURCES)
  list(JOIN SOURCES "\n" SOURCES)
  file(WRITE ${CMAKE_BINARY_DIR}/sources.txt "${SOURCES}\n")
  add_custom_target(format COMMAND ${CLANG_FORMAT} --verbose -i @${CMAKE_BINARY_DIR}/sources.txt)
endif ()
check_for_missing_sources()
