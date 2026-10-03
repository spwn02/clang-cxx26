// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -verify -verify-ignore-unexpected=note %s

// [stmt.expand]: an iterating expansion statement is 'constexpr decltype(auto)
// range = (expansion-initializer)'. For an lvalue range the constexpr reference
// must be constexpr-referenceable (P2686R5); otherwise the program is
// ill-formed rather than iterating a copy.
struct Range {
  int d[2]{1, 2};
  constexpr const int *begin() const { return d; }
  constexpr const int *end() const { return d + 2; }
};

thread_local constexpr Range tl;
consteval void thread_local_range() {
  template for (constexpr auto x : tl) { (void)x; } // expected-error {{constexpr variable '__range' must be initialized by a constant expression}} expected-error {{could not compute size of expansion}}
}

consteval int outer_local_in_lambda() {
  constexpr Range outer;
  auto l = [] {
    int t = 0;
    template for (constexpr auto x : outer) { t += x; } // expected-error {{variable 'outer' cannot be implicitly captured}} expected-error {{constexpr variable '__range' must be initialized by a constant expression}} expected-error {{could not compute size of expansion}}
    return t;
  };
  return l();
}

consteval int same_scope_and_lambda_own_local() {
  constexpr Range local;
  int t = 0;
  template for (constexpr auto x : local) { t += x; } // OK: same function parameter scope
  auto l = [] {
    constexpr Range inner;
    int u = 0;
    template for (constexpr auto x : inner) { u += x; } // OK: the lambda's own local
    return u;
  };
  return t + l();
}
static_assert(same_scope_and_lambda_own_local() == 6);
