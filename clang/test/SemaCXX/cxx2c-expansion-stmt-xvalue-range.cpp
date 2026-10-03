// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -freflection -fexpansion-statements %s
// expected-no-diagnostics
// [stmt.expand]: the range variable is `decltype(auto) range = (expansion-initializer)`: an lvalue gives T&,
// an xvalue gives T&& (both denote the original object), a prvalue is materialized by value.
struct R { int d[1]; constexpr const int *begin() const { return d; } constexpr const int *end() const { return d + 1; } };
constexpr R g{{5}};
consteval bool xvalue_range_identity() {
  bool ok = false;
  template for (constexpr auto &x : static_cast<const R &&>(g)) { ok = (&x == &g.d[0]); }
  return ok;
}
static_assert(xvalue_range_identity());   // decltype(auto) range = (xvalue) is R&&: no copy
consteval int prvalue_range() {
  int t = 0;
  template for (constexpr auto x : R{{3}}) { t += x; }
  return t;
}
static_assert(prvalue_range() == 3);
