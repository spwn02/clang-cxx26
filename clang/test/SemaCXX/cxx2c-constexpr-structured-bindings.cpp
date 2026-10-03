// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// P2686R5: constexpr structured bindings, including references to automatic objects.

struct S { int a; int b; };
constexpr auto [ga, gb] = S{1, 2};
static_assert(ga == 1 && gb == 2);

struct Pair {
  int first, second;
  template <int I> constexpr const int &get() const { return I == 0 ? first : second; }
};
namespace std {
template <class T> struct tuple_size;
template <int I, class T> struct tuple_element;
template <> struct tuple_size<Pair> { static constexpr int value = 2; };
template <int I> struct tuple_element<I, Pair> { using type = const int; };
}

consteval int local_bindings() {
  constexpr auto [x, y] = S{3, 4};
  static_assert(x == 3 && y == 4);
  constexpr S s{5, 6};
  constexpr auto &[p, q] = s;
  static_assert(p == 5 && q == 6);
  int arr[2] = {7, 8};
  constexpr auto &[r0, r1] = arr; // references to automatic array elements
  constexpr Pair pr{9, 10};
  constexpr auto &[u, v] = pr;    // tuple-like protocol, reference to an automatic object
  static_assert(u == 9 && v == 10);
  return x + y + p + q + u + v;
}
static_assert(local_bindings() == 3 + 4 + 5 + 6 + 9 + 10);

consteval int lambda_values() {
  constexpr auto [x, y] = S{1, 2};
  return [] { return x + y; }(); // values only: usable without a capture
}
static_assert(lambda_values() == 3);

consteval int lambda_reference() {
  constexpr S s{1, 2};
  constexpr auto &[x, y] = s;
  return [&] { return x + y; }(); // needs the capture-default
}
static_assert(lambda_reference() == 3);

consteval int not_constant() {
  int arr[2] = {7, 8};
  constexpr auto [c0, c1] = arr; // expected-error {{constexpr variable '[c0, c1]' must be initialized by a constant expression}}
  // expected-note@-1 {{read of non-constexpr variable 'arr' is not allowed in a constant expression}}
  // expected-note@-3 {{declared here}}
  return 0;
}
