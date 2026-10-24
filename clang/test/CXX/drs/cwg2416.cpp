// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg2416 { // cwg2416: yes
#if __cplusplus >= 201103L
namespace p1 {
template<class T> constexpr int f() { return 1; }
template<> int f<int>() { return 2; }
constexpr int a = f<char>();
}
namespace p2 {
template<class T> constexpr int f() { return 1; }
template<> int f<int>() { return 2; }
constexpr int b = f<int>();
// expected-error@-1 {{}}
// expected-note@*:* 0+ {{}}
}
#endif
} // namespace cwg2416
