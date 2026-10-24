// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace cwg2485 { // cwg2485: yes
#if __cplusplus >= 201703L
namespace p1 {
template<class A, class B> inline constexpr bool same = __is_same(A, B);
struct S { unsigned a : 4; unsigned long b : 33; unsigned c : 32; long long d : 40; int e : 5; bool f : 1; };
S s;
static_assert(same<decltype(+s.a), int>);
static_assert(same<decltype(+s.b), unsigned long>);
static_assert(same<decltype(+s.c), unsigned>);
static_assert(same<decltype(+s.d), long long>);
static_assert(same<decltype(+s.e), int>);
static_assert(same<decltype(+s.f), int>);
}
#endif
} // namespace cwg2485
