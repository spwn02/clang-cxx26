// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify -I %S/../Reflection/Inputs %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify -fexperimental-new-constant-interpreter -I %S/../Reflection/Inputs %s
// expected-no-diagnostics

// decltype of a resolved macro invocation is decltype of its expansion: an
// unparenthesized name stays an id-expression.
#include "token-sequence.h"
int g; struct S { int m; } s;
__macro mg() { return ^^{ g }; }
__macro ms() { return ^^{ s.m }; }
__macro mp() { return ^^{ (g) }; }
static_assert(__is_same(decltype(g), int));
static_assert(__is_same(decltype(mg!()), int));
static_assert(__is_same(decltype(ms!()), int));
static_assert(__is_same(decltype(mp!()), int&));
decltype(auto) f1() { return mg!(); }
decltype(auto) f2() { return mp!(); }
static_assert(__is_same(decltype(f1()), int));
static_assert(__is_same(decltype(f2()), int&));

__macro twice(int x) { return ^^{ (\(x) * 2) }; }
static_assert(twice!(21) == 42);
constexpr int cv = twice!(4);
static_assert(cv == 8);

// A template's own variables are instantiated inside an expansion.
__macro plus_one(int x) { return ^^{ (\(x) + 1) }; }
template <class T> constexpr int inst(int k) {
  int l = k;
  return plus_one!(k) + plus_one!(l);
}
static_assert(inst<int>(3) == 8);
