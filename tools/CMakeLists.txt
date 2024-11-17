cmake_minimum_required(VERSION 3.28)
unset(CMAKE_C_COMPILER_TARGET)
unset(CMAKE_CXX_COMPILER_TARGET)
unset(CMAKE_ASM_COMPILER_TARGET)
unset(CMAKE_SYSTEM)
set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})
unset(CMAKE_SHARED_LIBRARY_SUFFIX)
unset(CMAKE_SHARED_LIBRARY_SUFFIX_CXX)
unset(CMAKE_SHARED_LIBRARY_PREFIX)
unset(CMAKE_SHARED_LIBRARY_PREFIX_CXX)
unset(CMAKE_ASM_OUTPUT_EXTENSION)
unset(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
unset(CMAKE_CXX_COMPILER_ID_PLATFORM_CONTENT)

get_cmake_property(all_vars VARIABLES)
foreach (var ${all_vars})
  if (var MATCHES "CMAKE_CXX.*")
    unset(${var})
  endif ()
endforeach ()

project(dvnetos-tools LANGUAGES CXX)

#get_cmake_property(all_vars VARIABLES)
#foreach (var ${all_vars})
#  if (var MATCHES "CMAKE_.*")
#    message(STATUS "CMAKE: ${var} ${${var}}")
#  endif ()
#endforeach ()

message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

find_package(Python 3 REQUIRED)

include(ExternalProject)
ExternalProject_Add(
    venv
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ${Python_EXECUTABLE} -m venv .
    BUILD_COMMAND bin/pip install clang
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)
ExternalProject_Get_Property(venv BINARY_DIR)
set(VENV_PATH ${BINARY_DIR})

#execute_process(
#    COMMAND ${Python_EXECUTABLE} -m venv venv
#    COMMAND ${VENV_PATH}/bin/pip install clang
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#)

if(USE_CLANG)
  get_filename_component(CLANG_DIR ${CLANG} DIRECTORY)
  get_filename_component(CLANG_DIR ${CLANG_DIR} DIRECTORY)
  set(CLANG_DIR ${CLANG_DIR}/lib/)
  message(STATUS "CLANG_DIR: ${CLANG_DIR}")
#  set(CMAKE_FIND_DEBUG_MODE TRUE)
  find_library(LIBCLANG_PATH NAMES clang PATHS ${CLANG_DIR} REQUIRED NO_CACHE)
endif ()

set(PYTHON_VENV ${CMAKE_COMMAND} -E env "LIBCLANG_PATH=${LIBCLANG_PATH}" -- ${VENV_PATH}/bin/python)
set(PYTHON_VENV ${CMAKE_COMMAND} -E env "LIBCLANG_PATH=${LIBCLANG_PATH}" -- ${VENV_PATH}/bin/python PARENT_SCOPE)
set(ENUM2STRING ${CMAKE_CURRENT_SOURCE_DIR}/enumToString.py PARENT_SCOPE)

message(STATUS "PYTHON_VENV: ${PYTHON_VENV}")
