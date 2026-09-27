// RUN: %clang_cc1 -std=c++26 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -verify %s

// P3533R2: a constexpr variable of a class with virtual bases survives a PCH
// round trip (the APValue keeps one slot per virtual base).

#ifndef HEADER
#define HEADER
struct V {
  int n;
  constexpr V(int n) : n(n) {}
  constexpr virtual int f() const { return n; }
};
struct L : virtual V {
  constexpr L() : V(1) {}
};
struct R : virtual V {
  constexpr R() : V(2) {}
};
struct D : L, R {
  constexpr D() : V(3) {}
  constexpr int f() const override { return 30 + n; }
};
constexpr D global;
constexpr int viaBase = static_cast<const V &>(global).f();
#else
// expected-no-diagnostics
static_assert(global.n == 3);
static_assert(global.L::n == 3);
static_assert(viaBase == 33);
constexpr D again;
static_assert(again.R::n == 3);
#endif
