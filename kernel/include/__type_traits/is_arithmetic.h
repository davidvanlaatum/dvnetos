#ifndef IS_ARITHMETIC_H
#define IS_ARITHMETIC_H

namespace std {
  namespace detail {
    template<typename T>
    struct is_arithmetic {
      static constexpr bool value = std::is_integral<T>::value || std::is_floating_point<T>::value;
    };
  } // namespace detail

  template<typename T>
  struct is_arithmetic : detail::is_arithmetic<T> {
  };
} // namespace std

#endif // IS_ARITHMETIC_H
