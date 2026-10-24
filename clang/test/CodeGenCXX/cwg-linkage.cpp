// RUN: %clang_cc1 -std=c++17 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

namespace std { struct type_info; }

// CWG1416: typeid refers to the type_info of the cv-unqualified type.
// CHECK-DAG: ret ptr @_ZTIi
const std::type_info *cwg1416() { return &typeid(const int); }

// CWG1603: members of a class in an unnamed namespace have internal linkage.
namespace {
struct S {
  void f();
  static int x;
};
void S::f() {}
int S::x;
}
// CHECK-DAG: define internal void @_ZN12_GLOBAL__N_11S1fEv
// CHECK-DAG: @_ZN12_GLOBAL__N_11S1xE = internal global
void cwg1603() { S().f(); (void)S::x; }

// CWG1686: a variable of non-volatile const-qualified type has internal linkage.
typedef const int CI;
CI c = 1;
// CHECK-DAG: @_ZL1c = internal constant i32 1
const int *cwg1686() { return &c; }

// CWG1112: a constexpr variable has internal linkage like a const one.
constexpr int k = 2;
// CHECK-DAG: @_ZL1k = internal constant i32 2
const int *cwg1112() { return &k; }
