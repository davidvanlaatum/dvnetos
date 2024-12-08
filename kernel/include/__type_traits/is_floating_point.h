#ifndef IS_FLOATING_POINT_H
#define IS_FLOATING_POINT_H

namespace std {
  template<class T>
  struct is_floating_point
      : std::integral_constant<bool,
                               // Note: standard floating-point types
                               std::is_same<float, typename std::remove_cv<T>::type>::value ||
                                   std::is_same<double, typename std::remove_cv<T>::type>::value ||
                                   std::is_same<long double, typename std::remove_cv<T>::type>::value
                               // Note: extended floating-point types (C++23, if supported)
                               // || std::is_same<std::float16_t, typename std::remove_cv<T>::type>::value
                               // || std::is_same<std::float32_t, typename std::remove_cv<T>::type>::value
                               // || std::is_same<std::float64_t, typename std::remove_cv<T>::type>::value
                               // || std::is_same<std::float128_t, typename std::remove_cv<T>::type>::value
                               // || std::is_same<std::bfloat16_t, typename std::remove_cv<T>::type>::value
                               > {};
} // namespace std
#endif // IS_FLOATING_POINT_H
