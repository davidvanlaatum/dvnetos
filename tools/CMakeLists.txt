get_cmake_property(all_vars VARIABLES)
foreach (var ${all_vars})
  if (var MATCHES "CMAKE_CXX.*")
    unset(${var})
  endif ()
endforeach ()

unset(CMAKE_TOOLCHAIN_FILE)
project(dvnetos-tools LANGUAGES CXX)

#get_cmake_property(all_vars VARIABLES)
#foreach (var ${all_vars})
#  if (var MATCHES "CMAKE_.*")
#    message(STATUS "CMAKE: ${var} ${${var}}")
#  endif ()
#endforeach ()

find_package(Python 3 REQUIRED)

include(ExternalProject)
ExternalProject_Add(
    venv
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ${Python_EXECUTABLE} -m venv .
    BUILD_COMMAND bin/pip install --editable ${CMAKE_CURRENT_SOURCE_DIR}
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)
ExternalProject_Get_Property(venv BINARY_DIR)
set(VENV_PATH ${BINARY_DIR})

get_filename_component(CLANG_DIR ${CLANG} DIRECTORY)
get_filename_component(CLANG_DIR ${CLANG_DIR} DIRECTORY)
set(CLANG_DIR ${CLANG_DIR}/lib/)

if (NOT LIBCLANG_PATH OR NOT EXISTS ${LIBCLANG_PATH})
  configure_file(${CMAKE_SOURCE_DIR}/CMake/find-libclang.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/libclang-find/CMakeLists.txt @ONLY)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libclang-find-build)
  execute_process(
      COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} ${CMAKE_CURRENT_BINARY_DIR}/libclang-find/
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libclang-find-build
      OUTPUT_VARIABLE OUTPUT
      RESULT_VARIABLE RESULT
  )
  if (NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to find libclang:\n${OUTPUT}")
  endif ()
  file(READ ${CMAKE_CURRENT_BINARY_DIR}/libclang-find-build/libclang.path PATH)
  set(LIBCLANG_PATH ${PATH} CACHE FILEPATH "Path to libclang" FORCE)
endif ()
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/libclang-find)
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/libclang-find-build)

set(PYTHON_VENV_BIN ${VENV_PATH}/bin/)
set(WARNING_COLLECTOR_COMMAND ${PYTHON_VENV_BIN}/warningcollector PARENT_SCOPE)
set(PYTHON_VENV ${CMAKE_COMMAND} -E env "LIBCLANG_PATH=${LIBCLANG_PATH}" -- ${PYTHON_VENV_BIN}/python)
set(PYTHON_VENV ${PYTHON_VENV} PARENT_SCOPE)
set(ENUM2STRING ${CMAKE_CURRENT_SOURCE_DIR}/enumToString.py PARENT_SCOPE)
set(LCOV_COBERTURA_PATH ${VENV_PATH}/bin/lcov_cobertura PARENT_SCOPE)
