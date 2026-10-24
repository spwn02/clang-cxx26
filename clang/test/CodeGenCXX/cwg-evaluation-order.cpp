// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

int f();
int g();
struct A { int x, y; };

// CWG1342: the init-declarators of a declaration are initialized in order.
// CHECK-LABEL: define {{.*}}@_Z7cwg1342v
// CHECK: call noundef i32 @_Z1fv
// CHECK: call noundef i32 @_Z1gv
void cwg1342() { int a = f(), b = g(); (void)a; (void)b; }

// CWG1030: the initializer-clauses of an aggregate initializer are evaluated
// in the order of the elements.
// CHECK-LABEL: define {{.*}}@_Z7cwg1030v
// CHECK: call noundef i32 @_Z1fv
// CHECK: call noundef i32 @_Z1gv
void cwg1030() { A a{f(), g()}; (void)a; }
