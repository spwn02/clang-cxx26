// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg2461 { // cwg2461: yes
#if __cplusplus >= 202002L
namespace p1 {
template<class T> concept C = sizeof(T); template<class T> requires C<T> void f(); void g(){ f<int>(); }
// expected-error@-1 2 {{}}
// expected-note@*:* 0+ {{}}
}
namespace p2 {
template<class T> requires (sizeof(T)) void f(); void g(){ f<int>(); }
// expected-error@-1 2 {{}}
// expected-note@*:* 0+ {{}}
}
namespace p3 {
template<class T> concept C = bool(sizeof(T)); template<class T> requires C<T> void f(){} void g(){ f<int>(); }
}
#endif
} // namespace cwg2461
