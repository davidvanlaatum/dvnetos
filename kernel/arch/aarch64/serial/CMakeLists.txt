if (TARGET kernel AND ARCH STREQUAL "aarch64")
  target_sources(kernel PRIVATE Serial.cpp Serial.h)
endif ()
