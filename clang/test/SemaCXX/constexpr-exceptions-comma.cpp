// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s


// P3068R6: an exception thrown by the left operand of a comma expression propagates; the
// right operand is not evaluated, also when it is an lvalue.

struct E {};
[[noreturn]] constexpr void thr() { throw E{}; }

struct S {
  int d[2];
  constexpr const int &at_lvalue(unsigned p) const { return p >= 2 ? (thr(), d[0]) : d[p]; }
  constexpr int at_prvalue(unsigned p) const { return p >= 2 ? (thr(), d[0]) : d[p]; }
};

constexpr bool lvalue_comma() {
  try {
    S s{};
    (void)s.at_lvalue(9);
    return false;
  } catch (E) {
    return true;
  }
}
static_assert(lvalue_comma());

constexpr bool prvalue_comma() {
  try {
    S s{};
    (void)s.at_prvalue(9);
    return false;
  } catch (E) {
    return true;
  }
}
static_assert(prvalue_comma());

constexpr int ignored_lhs(bool b) {
  int n = 0;
  try {
    n = (b ? (thr(), 1) : 2, 3); // expected-warning {{left operand of comma operator has no effect}}
  } catch (E) {
    return 10;
  }
  return n;
}
static_assert(ignored_lhs(true) == 10 && ignored_lhs(false) == 3);
