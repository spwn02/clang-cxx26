// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// expected-no-diagnostics
template<class T> struct S {
  static constexpr auto r = ^^S::template S;
};
static_assert(S<int>::r == ^^S);
