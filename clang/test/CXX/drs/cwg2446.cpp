// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace cwg2446 { // cwg2446: 22
// A requires-expression and a concept-id are never type-dependent, so decltype
// of one is bool.
template <typename T> concept C = true;
template <typename T> struct A;
template <> struct A<bool> { using type = bool; };
template <typename T> void f(typename A<decltype(C<T>)>::type);
template <typename T> void g(T) { static_assert(__is_same(decltype(C<T>), bool), ""); }
void h() { f<int>(true); g(1); }
} // namespace cwg2446
