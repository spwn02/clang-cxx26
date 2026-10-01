// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -fparameter-reflection -verify
// expected-no-diagnostics

using info = decltype(^^int);
struct sentinel {};
consteval info annotation(info r, unsigned i) {
  return __metafunction(114, r, ^^sentinel, i, ^^annotation);
}
consteval int value(info r, unsigned i) {
  return __metafunction(25, ^^int, annotation(r, i), ^^value);
}
consteval info parameter(info r) {
  return __metafunction(107, r, ^^sentinel, 0, ^^parameter);
}
consteval info variable(info r) {
  return __metafunction(113, r, ^^variable);
}

[[=10]] void h();
[[=20]] void h();
static_assert(value(^^h, 0) == 10);
static_assert(value(^^h, 1) == 20);
static_assert(annotation(^^h, 2) == ^^sentinel);

[[=2, =3, =2]] void g();
void g [[=4, =5]] ();
static_assert(value(^^g, 0) == 2);
static_assert(value(^^g, 1) == 3);
static_assert(value(^^g, 2) == 2);
static_assert(value(^^g, 3) == 4);
static_assert(value(^^g, 4) == 5);
static_assert(annotation(^^g, 5) == ^^sentinel);

void p([[=1]] int x);
void p([[=2]] int y) {
  constexpr auto rp = parameter(^^p);
  constexpr auto ry = variable(rp);
  static_assert(ry == ^^y);
  static_assert(value(rp, 0) == 1);
  static_assert(value(rp, 1) == 2);
  static_assert(annotation(rp, 2) == ^^sentinel);
  static_assert(value(ry, 0) == 2);
  static_assert(annotation(ry, 1) == ^^sentinel);
  static_assert(annotation(rp, 1) == annotation(ry, 0));
}

template<class T> [[=1]] void ft(T);
template<class T> void ft [[=2]] (T) {}
template<> [[=3]] void ft<int>(int) {}
static_assert(value(^^ft<long>, 0) == 1);
static_assert(value(^^ft<long>, 1) == 2);
static_assert(annotation(^^ft<long>, 2) == ^^sentinel);
static_assert(value(^^ft<int>, 0) == 1);
static_assert(value(^^ft<int>, 1) == 2);
static_assert(value(^^ft<int>, 2) == 3);
static_assert(annotation(^^ft<int>, 3) == ^^sentinel);

template<class T> [[=int(sizeof(T))]] void dependent(T);
template<class T> void dependent [[=int(sizeof(T)) + 1]] (T) {}
static_assert(value(^^dependent<char>, 0) == 1);
static_assert(value(^^dependent<char>, 1) == 2);
static_assert(annotation(^^dependent<char>, 2) == ^^sentinel);

// Explicit instantiations are excluded from S(F).
template<class T> [[=1]] void explicitly_instantiated(T) {}
template void explicitly_instantiated<char> [[=9]] (char);
static_assert(value(^^explicitly_instantiated<char>, 0) == 1);
static_assert(annotation(^^explicitly_instantiated<char>, 1) == ^^sentinel);
