#ifndef REMOVE_CV_H
#define REMOVE_CV_H

namespace std {
  template<class T>
  struct remove_cv {
    typedef T type;
  };
  template<class T>
  struct remove_cv<const T> {
    typedef T type;
  };
  template<class T>
  struct remove_cv<volatile T> {
    typedef T type;
  };
  template<class T>
  struct remove_cv<const volatile T> {
    typedef T type;
  };

  template<class T>
  struct remove_const {
    typedef T type;
  };
  template<class T>
  struct remove_const<const T> {
    typedef T type;
  };

  template<class T>
  struct remove_volatile {
    typedef T type;
  };
  template<class T>
  struct remove_volatile<volatile T> {
    typedef T type;
  };
} // namespace std

#endif // REMOVE_CV_H
