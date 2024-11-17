function(enum2string)
  cmake_parse_arguments(ARG "" "HEADER;TARGET" "" ${ARGN})
  if (NOT ARG_HEADER)
    message(FATAL_ERROR "enum2string: HEADER not specified")
  endif ()
  if (NOT ARG_TARGET)
    message(FATAL_ERROR "enum2string: TARGET not specified")
  endif ()
  get_filename_component(HEADER_NAME ${ARG_HEADER} NAME_WE)
  message(STATUS "enum2string: HEADER_NAME: ${HEADER_NAME}")
  add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}_tostring.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}_tostring.h
      COMMAND ${PYTHON_VENV} ${ENUM2STRING} ${ARG_HEADER} ${HEADER_NAME}_tostring.cpp ${HEADER_NAME}_tostring.h -std=c++$<TARGET_PROPERTY:${ARG_TARGET},CXX_STANDARD> "-I$<JOIN:$<TARGET_PROPERTY:${ARG_TARGET},INCLUDE_DIRECTORIES>,;-I>"
      COMMAND_EXPAND_LISTS
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${ARG_HEADER} ${ENUM2STRING}
  )
  add_custom_target(generate-${HEADER_NAME}-tostring DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}_tostring.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_NAME}_tostring.h)
  add_dependencies(${ARG_TARGET} generate-${HEADER_NAME}-tostring)
  target_sources(${ARG_TARGET} PRIVATE ${HEADER_NAME}_tostring.cpp ${HEADER_NAME}_tostring.h)
  add_dependencies(generate-${HEADER_NAME}-tostring venv)
endfunction()
