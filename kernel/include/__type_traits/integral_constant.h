#ifndef INTEGRAL_CONSTANT_H
#define INTEGRAL_CONSTANT_H

namespace std {
  template<class T, T v>
  struct integral_constant {
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant; // using injected-class-name
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; } // since c++14
  };

  using true_type = std::integral_constant<bool, true>;
  using false_type = std::integral_constant<bool, false>;
  template<bool B>
  using bool_constant = integral_constant<bool, B>;
}

#endif //INTEGRAL_CONSTANT_H
