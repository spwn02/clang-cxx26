// RUN: %clang_analyze_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -analyzer-checker=core,debug.DumpCFG %s 2>&1 | FileCheck %s
// RUN: %clang_analyze_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -analyzer-checker=core,debug.ExprInspection -verify %s

#include "token-sequence.h"

void clang_analyzer_eval(bool);

// Token sequences are compile-time only: the CFG and the analyzer see the
// ordinary code around them, and the nodes themselves do not upset either.
consteval std::meta::token_sequence make(int n) { return ^^{ x = \(n) + 1 }; }

constexpr std::meta::token_sequence ts = ^^{ a + b };

int runtime(int k) {
  int r = k * 2;
  clang_analyzer_eval(r == k * 2); // expected-warning {{TRUE}}
  return r;
}
// CHECK: runtime
// CHECK: [B1]
// CHECK:   {{.*}} k * 2

template <class T> consteval std::meta::token_sequence dependent(T t) {
  return ^^{ \(t) + 1 };
}

// A resolved macro invocation is its expansion for the analyzer. (The value
// of an argument that is not a constant is not tracked through the opaque
// value the expansion holds: it is unknown, not wrong.)
__macro plus_one(int x) { return ^^{ (\(x) + 1) }; }
int through_macro(int k) {
  int r = plus_one!(k);
  (void)r;
  int q = plus_one!(41);
  clang_analyzer_eval(q == 42); // expected-warning {{TRUE}}
  return q;
}
