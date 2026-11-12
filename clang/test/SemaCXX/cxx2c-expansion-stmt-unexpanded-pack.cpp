// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// An expansion-init-list element that names a function parameter pack without
// expanding it used to reach the instantiator and hit an assertion in
// LocalInstantiationScope::findInstantiationOf.

template <class... Ts> consteval int f(Ts &&...ts) {
  int n = 0;
  template for (const auto &t : {ts}) { n += 1; } // expected-error {{expression contains unexpanded parameter pack 'ts'}}
  return n;
}

template <class... Ts> consteval int g(Ts &&...ts) {
  // The pack is declared outside the lambda, so the lambda carries it and is
  // diagnosed as a whole.
  auto l = [&](int) -> int { // expected-error {{initializer contains unexpanded parameter pack 'ts'}}
    int n = 0;
    template for (const auto &t : {ts}) { n += 1; }
    return n;
  };
  return l(0);
}

template <class... Ts> consteval int ok(Ts... ts) {
  int n = 0;
  template for (const auto &t : {ts...}) { n += 1; }
  return n;
}
static_assert(ok(1, 2, 3) == 3);

// A lambda that is itself expanded makes the pack legitimate: the expansion
// statement is instantiated once per pack element.
template <class... Ts> consteval int expanded_lambda(Ts... ts) {
  int total = 0;
  ((total += [&] {
     int n = 0;
     template for (const auto &t : {ts}) { n += t; }
     return n;
   }()), ...);
  return total;
}
static_assert(expanded_lambda(1, 2, 3) == 6);
