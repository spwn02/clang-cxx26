// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=pre-cxx20 -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=pre-cxx20 -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=pre-cxx20 -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=pre-cxx20 -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=since-cxx20 -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=since-cxx20 -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=since-cxx20 -fexceptions -fcxx-exceptions
// pre-cxx20-no-diagnostics

namespace cwg2237 { // cwg2237: 22
template<typename T> struct A {
  A();
  A(int);
  A<T>(long);
  // since-cxx20-error@-1 {{ISO C++20 considers use of template-id as the declarator name of a constructor to be ill-formed}}
};
template<typename T> A<T>::A(int) {}
}
