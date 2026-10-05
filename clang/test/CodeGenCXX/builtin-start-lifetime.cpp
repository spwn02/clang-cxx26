// RUN: %clang_cc1 -std=c++2c -triple x86_64-linux-gnu -emit-llvm -O0 -o - %s | FileCheck %s

// __builtin_start_lifetime only matters to the constant evaluator; at run time it does nothing.

struct A {
  int a;
  int b;
};

// CHECK-LABEL: define {{.*}} @_Z1fP1A
// CHECK-NOT: call
// CHECK: ret void
void f(A* p) { __builtin_start_lifetime(p); }

// The argument is still evaluated.
// CHECK-LABEL: define {{.*}} @_Z1gv
// CHECK: call {{.*}} @_Z4nextv
void g() {
  extern A* next();
  __builtin_start_lifetime(next());
}
