// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}

namespace cwg1478 { // cwg1478: yes
namespace p1 {
template<template<class> class TT> struct X {};
template<class T> struct Y { X<T::template A> x; };
struct Z { template<class> struct A {}; };
Y<Z> y;
}
namespace p2 {
template<template<class> class TT> struct X {};
template<class T> struct Y { X<T::A> x; };
struct Z { template<class> struct A {}; };
Y<Z> y;
// expected-error@-3 {{}}
}
} // namespace cwg1478
