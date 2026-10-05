// RUN: %clang_cc1 -std=c++2c -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++2c -include-pch %t -fsyntax-only -verify %s
// expected-no-diagnostics

// Defaulted postfix increment and decrement operators survive a PCH round trip.

#ifndef HEADER
#define HEADER

struct I {
  int v = 0;
  constexpr I& operator++() { ++v; return *this; }
  constexpr I operator++(int) = default;
};
struct E {
  int v = 0;
};
constexpr E& operator++(E& e) { ++e.v; return e; }
constexpr E operator++(E&, int) = default;
template <class T> struct W {
  T t{};
  constexpr W& operator++() { ++t; return *this; }
  constexpr W operator++(int) = default;
};

#else

static_assert([] { I i; I o = i++; return o.v == 0 && i.v == 1; }());
static_assert([] { E e; E o = e++; return o.v == 0 && e.v == 1; }());
static_assert([] { W<int> w; W<int> o = w++; return o.t == 0 && w.t == 1; }());

#endif
