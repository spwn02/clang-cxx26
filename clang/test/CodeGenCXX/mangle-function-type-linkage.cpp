// RUN: %clang_cc1 -std=c++17 -emit-llvm -triple x86_64-linux-gnu -o - %s | FileCheck %s

// [dcl.link]: function types with different language linkages are distinct
// types; the Itanium ABI mangles the one with C language linkage with 'Y'.
extern "C" typedef void CFn(int);
typedef void CxxFn(int);

// CHECK-LABEL: define {{.*}}void @_Z1fPFYviE(
void f(CFn *) {}
// CHECK-LABEL: define {{.*}}void @_Z1fPFviE(
void f(CxxFn *) {}
// CHECK-LABEL: define {{.*}}void @_Z1fRFYviE(
void f(CFn &) {}

template <class T> void t(T *) {}
// CHECK-LABEL: define {{.*}}void @_Z1tIFYviEEvPT_(
template void t<CFn>(CFn *);
// CHECK-LABEL: define {{.*}}void @_Z1tIFviEEvPT_(
template void t<CxxFn>(CxxFn *);

// A function with C language linkage is not mangled; its type is.
extern "C" void cfn(int) {}
// CHECK-LABEL: define {{.*}}void @cfn(
void use() { f(&cfn); }
// CHECK: call void @_Z1fPFYviE(
