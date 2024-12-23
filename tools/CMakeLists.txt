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

find_package(Python 3 REQUIRED)

include(ExternalProject)
ExternalProject_Add(
    venv
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ${Python_EXECUTABLE} -m venv .
    BUILD_COMMAND bin/pip install clang lcov_cobertura setuptools qemu.qmp
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)
ExternalProject_Get_Property(venv BINARY_DIR)
set(VENV_PATH ${BINARY_DIR})

get_filename_component(CLANG_DIR ${CLANG} DIRECTORY)
get_filename_component(CLANG_DIR ${CLANG_DIR} DIRECTORY)
set(CLANG_DIR ${CLANG_DIR}/lib/)
#  set(CMAKE_FIND_DEBUG_MODE TRUE)
find_library(LIBCLANG_PATH NAMES clang PATHS ${CLANG_DIR} REQUIRED NO_CACHE)

set(PYTHON_VENV ${CMAKE_COMMAND} -E env "LIBCLANG_PATH=${LIBCLANG_PATH}" -- ${VENV_PATH}/bin/python)
set(PYTHON_VENV ${PYTHON_VENV} PARENT_SCOPE)
set(ENUM2STRING ${CMAKE_CURRENT_SOURCE_DIR}/enumToString.py PARENT_SCOPE)
set(LCOV_COBERTURA_PATH ${VENV_PATH}/bin/lcov_cobertura PARENT_SCOPE)
