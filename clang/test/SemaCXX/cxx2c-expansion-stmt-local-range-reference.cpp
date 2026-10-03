// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -freflection-latest -fexpansion-statements %s
// expected-no-diagnostics
// P2686R5 / #188: a constexpr expansion variable binds the original range lvalue when it is
// constexpr-referenceable (an automatic object of the same function), instead of a copy.
struct R { int d[1]; constexpr const int *begin() const { return d; } constexpr const int *end() const { return d + 1; } };
consteval bool local_range_identity() {
  constexpr R r{{5}};
  bool ok = false;
  template for (constexpr auto &x : r) { ok = (&x == &r.d[0]); }
  return ok;
}
static_assert(local_range_identity());   // P2686R5: the range lvalue is bound by reference
constexpr R g{{7}};
consteval bool global_range_identity() {
  bool ok = false;
  template for (constexpr auto &x : g) { ok = (&x == &g.d[0]); }
  return ok;
}
static_assert(global_range_identity());
consteval int lambda_outer() {
  constexpr R r{{3}};
  auto l = [] { constexpr R in{{4}}; int t = 0; template for (constexpr auto x : in) { t += x; } return t; };
  int t = 0;
  template for (constexpr auto x : r) { t += x; }
  return t + l();
}
static_assert(lambda_outer() == 7);
