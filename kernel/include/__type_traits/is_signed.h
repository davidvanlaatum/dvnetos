#ifndef IS_SIGNED_H
#define IS_SIGNED_H

namespace std {
  namespace detail {
    template<typename T, bool = std::is_arithmetic<T>::value>
    struct is_signed : std::integral_constant<bool, T(-1) < T(0)> {};

    template<typename T>
    struct is_signed<T, false> : std::false_type {};
  } // namespace detail

  template<typename T>
  struct is_signed : detail::is_signed<T>::type {};

  template<class T>
  constexpr bool is_signed_v = is_signed<T>::value;
} // namespace std
#endif // IS_SIGNED_H
