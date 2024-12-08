#ifndef IS_INTEGRAL_H
#define IS_INTEGRAL_H

namespace std {
  template<class T>
  struct is_integral : std::bool_constant<
        requires(T t, T *p, void (*f)(T)) // T* parameter excludes reference types
        {
          reinterpret_cast<T>(t); // Exclude class types
          f(0); // Exclude enumeration types
          p + t; // Exclude everything not yet excluded but integral types
        }> {
  };
}

#endif //IS_INTEGRAL_H
