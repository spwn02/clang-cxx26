// RUN: %clang_cc1 -std=c++26 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -fsyntax-only -verify %s

// P3865R3: a deduced class type that designates a type template template parameter survives a PCH round trip, and is
// instantiated again in the consumer.

#ifndef HEADER
#define HEADER

template <typename... Ts> struct Y {
  Y();
  Y(Ts...);
};
template <template <typename T = char> class X> auto f() {
  X x{1};
  return x;
}
template <template <typename T = char> class X> auto g() {
  X x;
  return x;
}
template auto f<Y>();

#else

template <class A, class B> struct is_same { static constexpr bool value = false; };
template <class A> struct is_same<A, A> { static constexpr bool value = true; };
static_assert(is_same<decltype(g<Y>()), Y<char>>::value, "");
static_assert(is_same<decltype(f<Y>()), Y<int>>::value, "");
// expected-no-diagnostics

#endif
