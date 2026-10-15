// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fexperimental-new-constant-interpreter %s
// expected-no-diagnostics

// [basic.life]: the lifetime of an object ends when the call of its destructor starts, so std::is_within_lifetime(this)
// in a destructor is well-formed and false; the members are destroyed after the body and are still within their
// lifetimes in it (#307).

struct M {
  bool *p;
  constexpr ~M() { if consteval { *p = __builtin_is_within_lifetime(this); } }
};
constexpr bool d1() { bool r = true; { M x{&r}; } return r; }
static_assert(!d1());

struct X {
  bool *p;
  M m;
  bool *q;
  constexpr ~X() {
    if consteval {
      *p = __builtin_is_within_lifetime(this);
      *q = __builtin_is_within_lifetime(&m);
    }
  }
};
constexpr bool d2() {
  bool a = true, b = false, c = true;
  { X x{&a, M{&c}, &b}; }
  return !a && b && !c;
}
static_assert(d2());

struct B {
  bool *pb;
  constexpr ~B() { if consteval { *pb = __builtin_is_within_lifetime(this); } }
};
struct D : B {
  bool *pd;
  constexpr D(bool *b, bool *d) : B{b}, pd(d) {}
  constexpr ~D() { if consteval { *pd = __builtin_is_within_lifetime(this); } }
};
constexpr bool d3() {
  bool b = true, d = true;
  { D x(&b, &d); }
  return !b && !d;
}
static_assert(d3());
