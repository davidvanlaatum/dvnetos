if (TARGET kernel AND ARCH STREQUAL "x86_64")
  target_sources(kernel PRIVATE
      Serial.cpp
      Serial.h
  )
endif ()
