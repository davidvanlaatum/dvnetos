#ifndef IS_SAME_H
#define IS_SAME_H

namespace std {
  template<typename T, typename U>
  struct is_same : std::false_type {};

  template<typename T>
  struct is_same<T, T> : std::true_type {};
} // namespace std

#endif // IS_SAME_H
