// RUN: %clang_cc1 -std=c++17 -verify %s
// RUN: %clang_cc1 -std=c++20 -verify %s
// RUN: %clang_cc1 -std=c++23 -verify %s
// RUN: %clang_cc1 -std=c++2c -verify %s

namespace cwg3159 {
struct Missing {};

template<class T> struct X {
  static inline int arr[] = {1, 2, T::error};
  // expected-error@-1 {{no member named 'error' in 'cwg3159::Missing'}}
};

using Pointer = decltype(+X<Missing>::arr);
// expected-note@-1 {{in instantiation of static data member 'cwg3159::X<cwg3159::Missing>::arr' requested here}}

template<class T> struct Fixed {
  static inline int arr[3] = {1, 2, T::error};
};
using FixedPointer = decltype(+Fixed<Missing>::arr);

template<class T> struct Unconverted {
  static inline int arr[] = {1, 2, T::error};
};
using UnconvertedArray = decltype(Unconverted<Missing>::arr);
} // namespace cwg3159
