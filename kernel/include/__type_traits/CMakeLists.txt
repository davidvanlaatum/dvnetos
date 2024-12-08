if (TARGET kernel)
  target_sources(kernel PRIVATE
      is_arithmetic.h
      is_signed.h
      is_integral.h
      is_floating_point.h
      integral_constant.h
      is_same.h
      remove_cv.h
  )
endif ()
