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

// A member is destroyed before the destructors of the bases run (#310): it is not within its lifetime in the body of a
// base class destructor.
struct M2 { int v; constexpr ~M2() {} };
struct B2 {
  bool *r;
  const M2 *mp;
  constexpr ~B2() { if consteval { *r = __builtin_is_within_lifetime(mp); } }
};
struct D2 : B2 {
  M2 m;
  constexpr D2(bool *r) : B2{r, &m}, m{1} {}
};
constexpr bool d4() { bool r = true; { D2 d(&r); } return r; }
static_assert(!d4());

// [basic.life]: the lifetime of an object of a class type ends when its destructor call starts, a trivial destructor
// included, and that of any other object when it is destroyed. The destructor of a class calls the destructors of its
// members before those of its bases ([class.dtor]), so in the body of a base class destructor a member of a class type
// is gone, while a member of any other type is still within its lifetime (#312).
struct T { int a; };
struct B3 {
  bool *r1, *r2;
  int *o;
  const int *ip;
  const T *tp;
  constexpr ~B3() {
    if consteval {
      *r1 = __builtin_is_within_lifetime(ip);
      *r2 = __builtin_is_within_lifetime(tp);
      *o = *ip;
    }
  }
};
struct D3 : B3 {
  int i;
  T t;
  constexpr D3(bool *a, bool *b, int *o) : B3{a, b, o, &i, &t}, i(3), t{4} {}
};
constexpr int d5() {
  bool a = false, b = true;
  int o = 0;
  { D3 d(&a, &b, &o); }
  return (a && !b) ? o : -1;
}
static_assert(d5() == 3);
