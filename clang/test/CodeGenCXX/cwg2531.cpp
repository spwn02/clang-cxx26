// RUN: %clang_cc1 -std=c++17 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

// CWG2531: constexpr is implicitly inline only on the first declaration of a
// static data member.
struct A {
  static const int x;
  static constexpr int y = 2;
};
constexpr int A::x = 1;

// CHECK: @_ZN1A1xE = constant i32 1
// CHECK-NOT: comdat
int use() { return A::x + A::y; }
