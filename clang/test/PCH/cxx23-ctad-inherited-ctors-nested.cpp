// RUN: %clang_cc1 -std=c++23 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++23 -include-pch %t -verify %s
// expected-no-diagnostics

#ifndef HEADER
#define HEADER

template <typename T> struct Base { Base(T); };
template <typename T> struct Outer {
  template <typename U> struct Inner : Base<U> { using Base<U>::Base; };
};

// Synthesizes the inherited guides of Outer<int>::Inner in the PCH.
Outer<int>::Inner header_use('x');

#else

Outer<int>::Inner a('c');
static_assert(__is_same(decltype(a), Outer<int>::Inner<char>));
Outer<long>::Inner b(1.5);
static_assert(__is_same(decltype(b), Outer<long>::Inner<double>));

#endif
