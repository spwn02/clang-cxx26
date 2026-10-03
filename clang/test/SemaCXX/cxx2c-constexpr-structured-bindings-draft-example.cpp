// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// [expr.const.init] example (P2686R5): constexpr structured bindings. std::tuple is a minimal stand-in.
namespace std {
template <class T> struct tuple_size;
template <int I, class T> struct tuple_element;
template <class... T> struct tuple;
template <class T> struct tuple<T> { T v; };
template <class T> struct tuple_size<tuple<T>> { static constexpr int value = 1; };
template <class T> struct tuple_element<0, tuple<T>> { using type = T; };
template <int I, class T> constexpr T &get(tuple<T> &t) { return t.v; }
template <int I, class T> constexpr const T &get(const tuple<T> &t) { return t.v; }
}
struct S {
  mutable int m;
  constexpr S(int m): m(m) {}
  virtual int g() const;
};
void f(std::tuple<S&> t) {
  auto [r] = t;
  static_assert(r.g() >= 0);            // expected-error {{static assertion expression is not an integral constant expression}}
  constexpr auto [m] = S(1);
  static_assert(m == 1);                // expected-error {{static assertion expression is not an integral constant expression}}
  using A = int[2];
  constexpr auto [v0, v1] = A{2, 3};
  static_assert(v0 + v1 == 5);          // OK
}
