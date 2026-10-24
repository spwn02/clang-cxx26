// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg1353 { // cwg1353: yes
#if __cplusplus >= 201103L
namespace p1 {
struct NT { NT() = delete; };
struct S { NT a[2]; };
S s;
// expected-error@-1 {{}}
}
namespace p2 {
struct NC { NC(); NC(const NC&) = delete; };
struct S { NC a[2]; };
void f(S& s) { S t(s); }
// expected-error@-1 {{}}
}
#endif
} // namespace cwg1353
