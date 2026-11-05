// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -ast-print %s | FileCheck %s

// A statement-position invocation is printed as its expansion, preceded by a
// comment naming the invocation: reading the output must not expand the macro
// a second time. An invocation inside an expression is printed as written.
#include "token-sequence.h"

__macro inc(int x) { return ^^{ (\(x) + 1) }; }

void f(int k) {
  inc!(k);
  int r = inc!(k);
}
// CHECK-LABEL: void f(int k) {
// CHECK-NEXT:    /* inc!(k) */;
// CHECK-NEXT:    (k + 1);
// CHECK-NEXT:    int r = inc!(k);
