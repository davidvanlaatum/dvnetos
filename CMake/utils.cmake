include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CMakePushCheckState)

function(add_compile_options_if_supported lang flag addToVar)
  cmake_push_check_state()
  string(JOIN " " CMAKE_REQUIRED_FLAGS "${${addToVar}}")
  string(REGEX REPLACE "\\+" "p" VAR "${lang}_COMPILER_SUPPORTS${flag}")
  string(REGEX REPLACE "[^A-Za-z0-9_]+" "_" VAR "${VAR}")
  check_compiler_flag(${lang} "-Werror ${flag}" ${VAR})
  if (${VAR})
    list(APPEND ${addToVar} "${flag}")
    set(${addToVar} "${${addToVar}}" PARENT_SCOPE)
  endif ()
  cmake_pop_check_state()
endfunction()

function(add_linker_options_if_supported lang flag addToVar)
  cmake_push_check_state()
  string(JOIN " " CMAKE_REQUIRED_FLAGS "${${addToVar}}")
  list(APPEND CMAKE_REQUIRED_LINK_OPTIONS ${${addToVar}} ${flag})
  string(REGEX REPLACE "[^A-Za-z_]+" "_" VAR "${lang}_LINKER_SUPPORTS${flag}")
  check_linker_flag(${lang} "${flag}" ${VAR})
  if (${VAR})
    list(APPEND ${addToVar} "${flag}")
    set(${addToVar} "${${addToVar}}" PARENT_SCOPE)
  endif ()
  cmake_pop_check_state()
endfunction()

function(getAllSubdirs dir dirs)
  get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
  message(TRACE "Subdirs in ${dir}: ${subdirs}")
  foreach (subdir ${subdirs})
    list(APPEND ${dirs} ${subdir})
    getAllSubdirs(${subdir} ${dirs})
  endforeach ()
  set(${dirs} ${${dirs}} PARENT_SCOPE)
endfunction()

function(getAllTargets var)
  set(${var} "")
  set(DIRS ${CMAKE_SOURCE_DIR})
  getAllSubDirs(. DIRS)
  foreach (dir ${DIRS})
    string(FIND ${dir} "${CMAKE_BINARY_DIR}/_deps" IS_DEPS_DIR)
    if (IS_DEPS_DIR EQUAL 0)
      message(TRACE "Skipping deps dir ${dir}")
      continue()
    endif ()
    get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    message(TRACE "Targets in ${dir}: ${targets}")
    foreach (target ${targets})
      get_target_property(TYPE ${target} TYPE)
      if (TYPE STREQUAL "UTILITY")
        message(TRACE "Skipping utility target ${target}")
        continue()
      endif ()
      list(APPEND ${var} ${target})
    endforeach ()
  endforeach ()
  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

function(getAllTests var)
  set(DIRS ${CMAKE_SOURCE_DIR})
  getAllSubdirs(. DIRS)
  foreach (dir ${DIRS})
    string(FIND ${dir} "${CMAKE_BINARY_DIR}/_deps" IS_DEPS_DIR)
    if (IS_DEPS_DIR EQUAL 0)
      continue()
    endif ()
    get_property(tests DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    foreach (test ${tests})
      get_target_property(IS_TEST_CASE ${test} IS_TEST_CASE)
      if (IS_TEST_CASE STREQUAL "ON")
        list(APPEND ${var} ${test})
      endif ()
    endforeach ()
  endforeach ()
  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

function(add_to_all_sources)
  set(NEW_SOURCES ${ARGN})
  get_directory_property(ALL_SOURCES DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} SOURCES)
  list(REMOVE_DUPLICATES NEW_SOURCES)
  list(FILTER NEW_SOURCES EXCLUDE REGEX "PRIVATE|PUBLIC|INTERFACE")
  foreach (source ${NEW_SOURCES})
    cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE S)
    list(APPEND ALL_SOURCES ${S})
  endforeach ()
  list(REMOVE_DUPLICATES ALL_SOURCES)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY SOURCES ${ALL_SOURCES})
endfunction()

function(cus_target_sources target)
  if (TARGET ${target})
    message(DEBUG "Adding sources to target ${target}: ${ARGN}")
    target_sources(${target} PRIVATE ${ARGN})
  endif ()
  add_to_all_sources(${ARGN})
endfunction()

function(arch_target_sources arch target)
  if (ARCH STREQUAL ${arch} AND TARGET ${target})
    message(DEBUG "Adding sources to target ${target}: ${ARGN}")
    target_sources(${target} PRIVATE ${ARGN})
  endif ()
  add_to_all_sources(${ARGN})
endfunction()

function(get_all_sources var)
  set(DIRS ${CMAKE_SOURCE_DIR})
  getAllSubdirs(. DIRS)
  foreach (dir ${DIRS})
    string(FIND ${dir} "${CMAKE_BINARY_DIR}/_deps" IS_DEPS_DIR)
    if (IS_DEPS_DIR EQUAL 0)
      continue()
    endif ()
    get_directory_property(${var}_d DIRECTORY ${dir} SOURCES)
    list(APPEND ${var} ${${var}_d})
  endforeach ()
  set(${var} ${${var}} PARENT_SCOPE)
endfunction()

function(check_for_missing_sources)
  get_all_sources(ALL_SOURCES)
  getAllTargets(all_targets)
  set(MISSING "")
  foreach (target ${all_targets})
    get_target_property(SOURCES ${target} SOURCES)
    get_target_property(SOURCE_DIR ${target} SOURCE_DIR)
    foreach (source ${SOURCES})
      cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY ${SOURCE_DIR} OUTPUT_VARIABLE source)
      if (NOT source IN_LIST ALL_SOURCES)
        list(APPEND MISSING ${source})
      endif ()
    endforeach ()
  endforeach ()
  set(DIRS ${CMAKE_SOURCE_DIR})
  getAllSubdirs(. DIRS)
  foreach (dir ${DIRS})
    string(FIND ${dir} "${CMAKE_BINARY_DIR}/_deps" IS_DEPS_DIR)
    if (IS_DEPS_DIR EQUAL 0)
      continue()
    endif ()
    file(GLOB sources ${dir}/*.cpp ${dir}/*.h)
    foreach (source ${sources})
      cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY ${dir} OUTPUT_VARIABLE source)
      if (NOT source IN_LIST ALL_SOURCES)
        list(APPEND MISSING ${source})
      endif ()
    endforeach ()
  endforeach ()
  if (MISSING)
    list(REMOVE_DUPLICATES MISSING)
    list(LENGTH MISSING MISSING_COUNT)
    message(FATAL_ERROR "Missing ${MISSING_COUNT} sources: ${MISSING}")
  endif ()
endfunction()
