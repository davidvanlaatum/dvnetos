if (NOT TEST_MODE)
  include(enum2string)
  enum2string(HEADER smbios.h TARGET kernel)
  set_source_files_properties(tables.cpp PROPERTIES GENERATED TRUE)
  target_sources(kernel PRIVATE
      smbios.cpp
      smbios.h
  )
endif ()
