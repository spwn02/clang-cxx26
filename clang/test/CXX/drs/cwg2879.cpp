// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg2879 { // cwg2879: yes
#if __cplusplus >= 201103L
namespace p1 {
int x = 0; int&& ok = const_cast<int&&>(static_cast<int&&>(x));
}
namespace p2 {
int&& bad = const_cast<int&&>(2);
// expected-error@-1 {{}}
// expected-note@*:* 0+ {{}}
}
namespace p3 {
int n = const_cast<int>(0);
// expected-error@-1 {{}}
// expected-note@*:* 0+ {{}}
}
#endif
} // namespace cwg2879
