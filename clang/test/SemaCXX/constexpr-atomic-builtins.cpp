// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// P3309R3: the atomic load, store, exchange and compare-exchange builtins are constant evaluated.

struct S {
  int a, b;
};
struct W {
  _Atomic(S) v;
  constexpr W(S s) : v(s) {}
};

constexpr bool c11() {
  W w(S{1, 2});
  S o = __c11_atomic_load(&w.v, __ATOMIC_SEQ_CST);
  __c11_atomic_store(&w.v, S{3, 4}, __ATOMIC_SEQ_CST);
  S x = __c11_atomic_exchange(&w.v, S{5, 6}, __ATOMIC_SEQ_CST);
  S e{5, 6};
  bool ok = __c11_atomic_compare_exchange_strong(&w.v, &e, S{7, 8}, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  S e2{0, 0};
  bool ok2 = __c11_atomic_compare_exchange_weak(&w.v, &e2, S{9, 9}, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return o.a == 1 && x.a == 3 && ok && !ok2 && e2.a == 7 && __c11_atomic_load(&w.v, __ATOMIC_SEQ_CST).b == 8;
}
static_assert(c11());

constexpr bool gnu() {
  S s{1, 2}, r;
  __atomic_load(&s, &r, __ATOMIC_SEQ_CST);
  S n{3, 4};
  __atomic_store(&s, &n, __ATOMIC_SEQ_CST);
  S d{7, 8}, old;
  __atomic_exchange(&s, &d, &old, __ATOMIC_SEQ_CST);
  S ex{7, 8}, de{9, 9};
  bool ok = __atomic_compare_exchange(&s, &ex, &de, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  int i = 4;
  __atomic_store_n(&i, 5, __ATOMIC_SEQ_CST);
  return r.a == 1 && old.a == 3 && ok && s.a == 9 && __atomic_load_n(&i, __ATOMIC_SEQ_CST) == 5 &&
         __atomic_exchange_n(&i, 6, __ATOMIC_SEQ_CST) == 5 && i == 6;
}
static_assert(gnu());

// The comparison is bitwise on the value, not operator==.
constexpr bool bitwise() {
  _Atomic(float) f = -0.0f;
  float z = 0.0f;
  return !__c11_atomic_compare_exchange_strong(&f, &z, 1.0f, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
static_assert(bitwise());

// The arithmetic read-modify-write builtins are not constant evaluated.
constexpr int add() { // expected-error {{constexpr function never produces a constant expression}}
  _Atomic(int) i = 1;
  return __c11_atomic_fetch_add(&i, 1, __ATOMIC_SEQ_CST); // expected-note {{subexpression not valid in a constant expression}}
}
