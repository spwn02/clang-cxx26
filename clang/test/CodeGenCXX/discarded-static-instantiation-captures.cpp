// RUN: %clang_cc1 -std=c++20 -fblocks -fopenmp -emit-llvm -o - -Wno-unused-value %s | FileCheck %s

// A discarded mention of a static data member does not instantiate its
// definition (the variable is only declared); the locals named in the same
// discarded expression inside blocks and OpenMP regions are still captured.

template <class T> struct Never { static int q; };
template <class T> int Never<T>::q = T::nope;
struct K { K(); K(const K &); ~K(); int v; };

// CHECK: @_ZN5NeverIiE1qE = external global i32
// CHECK-NOT: T::nope

// CHECK-LABEL: define {{.*}} @_Z2f1v
// CHECK: call void ({{.*}}) @__kmpc_fork_call({{.*}}, ptr @_Z2f1v.omp_outlined, ptr %x
void f1() {
  int x = 1;
  _Pragma("omp parallel") { (Never<int>::q, x); }
}

// CHECK-LABEL: define {{.*}} @_Z2g1v
// CHECK: %block.captured = getelementptr {{.*}}%block, i32 0, i32 5
void g1() {
  int x = 1;
  ^{ (Never<int>::q, x); }();
}
