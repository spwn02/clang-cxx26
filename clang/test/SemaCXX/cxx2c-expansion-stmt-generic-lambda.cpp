// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -verify %s

// Expansion statements are equivalent to compound statements [stmt.expand].
// Their DeclContexts must not introduce a lambda capture boundary.
constexpr int enumerating() {
  auto f = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  return f(1);
}
static_assert(enumerating() == 2);

struct Pair { int first, second; };
constexpr int destructuring() {
  auto f = [](auto const &t) {
    int n = 0;
    template for (auto x : t) { n += x; }
    return n;
  };
  return f(Pair{1, 2});
}
static_assert(destructuring() == 3);

struct Range {
  int values[2];
  constexpr const int *begin() const { return values; }
  constexpr const int *end() const { return values + 2; }
};
constexpr int constexpr_initializers() {
  auto f = [](auto v) {
    constexpr int a = 1, b = 2;
    constexpr Pair p{a, b};
    constexpr Range r{{a, b}};
    int n = v;
    template for (constexpr auto x : {a, b}) { n += x; }
    template for (constexpr auto x : p) { n += x; }
    template for (constexpr auto x : r) { n += x; }
    return n;
  };
  return f(0);
}
static_assert(constexpr_initializers() == 9);

constexpr int captures() {
  int n = 0;
  auto by_ref = [&](auto v) {
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  if (by_ref(1) != 2 || n != 2) return -1;
  auto explicit_copy = [n](auto v) mutable {
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  auto default_copy = [=](auto v) mutable {
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  return explicit_copy(1) + default_copy(2) + n;
}
static_assert(captures() == 12);

constexpr int nested() {
  auto f = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) {
      template for (auto y : {x, x}) {
        n += y;
      }
      auto add = [&] { n += x; };
      add();
      auto copy = [x](auto z) { return x + z; };
      n += copy(0);
    }
    return n;
  };
  return f(1);
}
static_assert(nested() == 8);

template<class T> constexpr int in_function_template(T v) {
  auto f = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  auto non_generic = [v] {
    int n = 0;
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  return f(v) + non_generic();
}
static_assert(in_function_template(1) == 4);

void negative_controls() {
  int outside = 0; // expected-note 4{{'outside' declared here}}
  auto ordinary = [] { // expected-note {{lambda expression begins here}} expected-note {{capture 'outside' by value}} expected-note {{capture 'outside' by reference}} expected-note {{default capture by value}} expected-note {{default capture by reference}}
    return outside; // expected-error {{variable 'outside' cannot be implicitly captured in a lambda with no capture-default specified}}
  };
  auto expanded = [](auto v) { // expected-note 3{{lambda expression begins here}} expected-note 3{{capture 'outside' by value}} expected-note 3{{capture 'outside' by reference}} expected-note 3{{default capture by value}} expected-note 3{{default capture by reference}}
    template for (auto x : {v, v}) { // expected-note 2{{in instantiation of expansion statement requested here}}
      outside += x; // expected-error 3{{variable 'outside' cannot be implicitly captured in a lambda with no capture-default specified}}
    }
  };
}
