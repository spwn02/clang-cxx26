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

// A resolved macro invocation is its expansion for the analyzer, and the values
// of its arguments are the values of the opaque values in the expansion.
__macro plus_one(int x) { return ^^{ (\(x) + 1) }; }
int through_macro(int k) {
  int r = plus_one!(k);
  clang_analyzer_eval(r == k + 1); // expected-warning {{TRUE}}
  int q = plus_one!(41);
  clang_analyzer_eval(q == 42); // expected-warning {{TRUE}}
  return q;
}

// Macros that invoke macros: each argument is evaluated once, where the
// expansion that uses it does.
__macro twice_plus_one(int x) { return ^^{ plus_one!(plus_one!(\(x))) }; }
int nested_macro(int k) {
  int r = twice_plus_one!(k);
  clang_analyzer_eval(r == k + 2); // expected-warning {{TRUE}}
  return r;
}

// An argument is evaluated where the expansion uses it, and not at all if the
// expansion does not get there: the right operand of an operator macro `or` is
// not evaluated when the left one decides, as in the generated code (#500). The
// analyzer asserted in VisitLogicalExpr: the CFG had evaluated both arguments
// ahead of the expansion.
__macro lor(bool a, bool b) { return ^^{ \(a) or \(b) }; }
__macro land(bool a, bool b) { return ^^{ \(a) and \(b) }; }
bool unknown();
bool short_circuit(bool x) { return lor!(x, unknown()); }
bool short_circuit_and(bool x) { return land!(x, unknown()); }

int calls;
bool counting() { ++calls; return true; }
void right_operand_skipped() {
  calls = 0;
  bool r = lor!(true, counting());
  clang_analyzer_eval(r); // expected-warning {{TRUE}}
  clang_analyzer_eval(calls == 0); // expected-warning {{TRUE}}
  r = land!(false, counting());
  clang_analyzer_eval(r); // expected-warning {{FALSE}}
  clang_analyzer_eval(calls == 0); // expected-warning {{TRUE}}
  r = lor!(false, counting());
  clang_analyzer_eval(calls == 1); // expected-warning {{TRUE}}
}

// A nested invocation inside the right operand is evaluated only when it is reached.
bool nested_in_operand(bool x) { return lor!(x, lor!(unknown(), unknown())); }

// An operator macro has no invocation node: its expansion replaces the
// operator expression, and the arguments are opaque values in it all the same.
struct Flag {
  bool b;
  __macro operator or(this Flag self, bool rhs) { return ^^{ \(self).b or \(rhs) }; }
};
bool operator_macro(bool x) { return Flag{x} or unknown(); }
void operator_macro_skips_right_operand() {
  calls = 0;
  bool r = Flag{true} or counting();
  clang_analyzer_eval(r); // expected-warning {{TRUE}}
  clang_analyzer_eval(calls == 0); // expected-warning {{TRUE}}
  r = Flag{false} or counting();
  clang_analyzer_eval(calls == 1); // expected-warning {{TRUE}}
}
