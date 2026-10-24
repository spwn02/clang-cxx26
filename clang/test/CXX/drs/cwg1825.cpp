// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg1825 { // cwg1825: yes
#if __cplusplus >= 201103L
namespace p1 {
template<class... T> int f(T...);
template<class T> long f(T);
static_assert(__is_same(decltype(f(1)), long), "");
template<class T, class... U> int g(T, U...);
template<class T> long g(T);
static_assert(__is_same(decltype(g(1)), long), "");
}
#endif
} // namespace cwg1825
