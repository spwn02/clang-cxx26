// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - -O1 -disable-llvm-passes %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -ast-print %s | FileCheck %s --check-prefix=PRINT

// An expansion statement over a braced list whose elements create temporaries with a non-trivial destructor wraps
// the range in ExprWithCleanups; it used to be diagnosed ("variable has incomplete type 'const void'") and, in a
// template, to crash.

struct D {
  D();
  D(const D &);
  ~D();
};
D make();

// CHECK-LABEL: define {{.*}} @_Z1fv(
// CHECK: call void @_ZN1DC1Ev
// CHECK: call void @_ZN1DD1Ev
// CHECK: call void @_ZN1DC1Ev
// CHECK: call void @_ZN1DD1Ev
// CHECK: ret void
// PRINT: template for (auto x : {D(), D()}) {
void f() {
  template for (auto x : {D(), D()}) {
    (void)x;
  }
}

// CHECK-LABEL: define {{.*}} @_Z1gv(
// CHECK: call {{.*}} @_Z4makev
// CHECK: call void @_ZN1DD1Ev
// CHECK: call {{.*}} @_Z4makev
// CHECK: call void @_ZN1DD1Ev
void g() {
  template for (auto x : {make(), make()}) {
    (void)x;
  }
}

// CHECK-LABEL: define {{.*}} @_Z1hIiEvv(
// PRINT: template for (auto x : {T(), T()}) {
template <class T>
void h() {
  template for (auto x : {T(), T()}) {
    (void)x;
  }
}
template void h<int>();

// CHECK-LABEL: define {{.*}} @_Z1hI1DEvv(
template void h<D>();
