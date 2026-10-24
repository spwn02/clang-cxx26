// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace cwg2806 { // cwg2806: 22
struct S { template<class U> struct X {}; };
template<class T> concept C = requires { typename T::X<int>; };
static_assert(C<S>);
}
