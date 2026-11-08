// RUN: %clang_cc1 -std=c++17 -ast-print %s | FileCheck %s

struct P { int a, b; };

// CHECK: auto [x, y] = P{1, 2};
auto [x, y] = P{1, 2};

// CHECK: void f() {
// CHECK-NEXT: const auto & [a, b] = P{3, 4};
void f() {
  const auto &[a, b] = P{3, 4};
}
