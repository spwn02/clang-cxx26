// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s

// P3533R2: constexpr virtual inheritance.

#if __cplusplus > 202302L
#  if __cpp_constexpr_virtual_inheritance != 202506L
#    error "wrong value for __cpp_constexpr_virtual_inheritance"
#  endif
#else
#  ifdef __cpp_constexpr_virtual_inheritance
#    error "__cpp_constexpr_virtual_inheritance must only be defined in C++26"
#  endif
#endif

#if __cplusplus > 202302L
// expected-no-diagnostics

struct A {
  int a = 1;
  constexpr virtual ~A() = default;
};
struct B : virtual A {};
struct C : B {};
constexpr int g() {
  C c;
  return c.a;
}
static_assert(g() == 1);

// The most derived class initializes the virtual base, exactly once.
struct V {
  int n;
  constexpr V(int n) : n(n) {}
};
struct L : virtual V {
  constexpr L() : V(1) {}
};
struct R : virtual V {
  constexpr R() : V(2) {}
};
struct D : L, R {
  constexpr D() : V(3) {}
};
static_assert(D().n == 3);
static_assert(D().L::n == 3 && D().R::n == 3);
static_assert(L().n == 1 && R().n == 2);

struct Cnt {
  int *p;
  constexpr Cnt(int *p) : p(p) { ++*p; }
};
struct L2 : virtual Cnt {
  constexpr L2(int *p) : Cnt(p) {}
};
struct R2 : virtual Cnt {
  constexpr R2(int *p) : Cnt(p) {}
};
struct D2 : L2, R2 {
  constexpr D2(int *p) : Cnt(p), L2(p), R2(p) {}
};
constexpr int count() {
  int c = 0;
  D2 d(&c);
  return c;
}
static_assert(count() == 1);

// Virtual bases are constructed before non-virtual ones.
struct Order {
  int *log;
  constexpr Order(int *log, int id) : log(log) { *log = *log * 10 + id; }
};
struct VB1 : virtual Order {
  constexpr VB1(int *l) : Order(l, 1) {}
};
struct NB2 : Order {
  constexpr NB2(int *l) : Order(l, 2) {}
};
struct Derived : NB2, VB1 {
  constexpr Derived(int *l) : Order(l, 3), NB2(l), VB1(l) {}
};
constexpr int construction_order() {
  int log = 0;
  Derived d(&log);
  return log;
}
// The virtual Order(3) first, then NB2's own non-virtual Order(2); VB1's
// mem-initializer for its virtual base is ignored.
static_assert(construction_order() == 32);

// Destruction: the complete object destroys its virtual bases last.
struct Log {
  int *log;
  int id;
  constexpr Log(int *l, int id) : log(l), id(id) {}
  constexpr ~Log() { *log = *log * 10 + id; }
};
struct P : virtual Log {
  constexpr P(int *l) : Log(l, 1) {}
  constexpr ~P() { *log = *log * 10 + 2; }
};
struct Q : virtual Log {
  constexpr Q(int *l) : Log(l, 1) {}
  constexpr ~Q() { *log = *log * 10 + 3; }
};
struct PQ : P, Q {
  constexpr PQ(int *l) : Log(l, 1), P(l), Q(l) {}
  constexpr ~PQ() { *log = *log * 10 + 4; }
};
constexpr int destruction_order() {
  int log = 0;
  {
    PQ pq(&log);
  }
  return log;
}
static_assert(destruction_order() == 4321); // ~PQ, ~Q, ~P, then ~Log

// Virtual dispatch through a virtual base, including a diamond whose final
// overrider is on the other arm.
struct VA {
  constexpr virtual int f() const { return 1; }
  constexpr virtual ~VA() = default;
};
struct VB : virtual VA {};
struct VC : virtual VA {
  constexpr int f() const override { return 3; }
};
struct VD : VB, VC {};
constexpr int call_through_vb() {
  VD d;
  const VB &b = d;
  const VA &a = b;
  return a.f();
}
static_assert(call_through_vb() == 3);
constexpr int call_direct() {
  VB b;
  const VA &a = b;
  return a.f();
}
static_assert(call_direct() == 1);

// Literal type, constexpr variable of such a class.
struct W {
  int x;
};
struct WD : virtual W {
  constexpr WD() : W{4} {}
};
constexpr WD wd;
static_assert(wd.x == 4);

// Data access through the virtual base of the base subobject.
struct Vx { int v = 5; };
struct M1 : virtual Vx {};
struct M2 : virtual Vx {};
struct MD : M1, M2 { constexpr MD() { v = 6; } };
constexpr int access() {
  MD m;
  M1 &a = m;
  M2 &b = m;
  return a.v * 10 + b.v;
}
static_assert(access() == 66);

#else
struct NV {
  int a;
  constexpr NV() : a(0) {}
};
struct WV : virtual NV { // cxx23-note {{virtual base class declared here}}
  constexpr WV() {} // cxx23-error {{constexpr constructor not allowed in struct with virtual base class}}
};
#endif
