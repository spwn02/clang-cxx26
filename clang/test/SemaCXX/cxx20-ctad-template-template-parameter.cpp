// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// expected-no-diagnostics

// Before C++26 (P3865R3) a placeholder that designates a type template template parameter deduces through the template
// argument itself, not through an alias template: every constructor of Y counts, whatever the parameter list of X.

template <class A, class B> struct is_same { static constexpr bool value = false; };
template <class A> struct is_same<A, A> { static constexpr bool value = true; };

template <typename... Ts> struct Y {
  Y(Ts...);
};

template <template <typename T = char> class X> void f() {
  X x{1, 2};
  static_assert(is_same<decltype(x), Y<int, int>>::value, "");
}
template void f<Y>();
