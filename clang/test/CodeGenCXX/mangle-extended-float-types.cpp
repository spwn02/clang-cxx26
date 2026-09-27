// RUN: %clang_cc1 -std=c++23 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

// P1467R9: Itanium mangling of the extended floating-point types matches GCC's _FloatN.

// CHECK-DAG: define {{.*}} @_Z1fDF32_(
// CHECK-DAG: define {{.*}} @_Z1fDF64_(
// CHECK-DAG: define {{.*}} @_Z1fDF16_(
// CHECK-DAG: define {{.*}} @_Z1fDF16b(
// CHECK-DAG: define {{.*}} @_Z1ff(
// CHECK-DAG: define {{.*}} @_Z1fd(
void f(__float32) {}
void f(__float64) {}
void f(_Float16) {}
void f(__bf16) {}
void f(float) {}
void f(double) {}

// CHECK-LABEL: define {{.*}} float @_Z5add32DF32_DF32_(
// CHECK: fadd float
__float32 add32(__float32 a, __float32 b) { return a + b; }
__float32 add3(__float32 a, __float32 b) { return add32(a, b); }

// CHECK-LABEL: define {{.*}} double @_Z5widenDF32_(
// CHECK: fpext float {{.*}} to double
double widen(__float32 a) { return a; }

// CHECK-LABEL: define {{.*}} double @_Z5mixedDF64_d(
// CHECK: fadd double
__float64 mixed(__float64 a, double b) { return a + b; }
