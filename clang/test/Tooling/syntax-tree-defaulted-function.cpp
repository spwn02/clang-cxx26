// RUN: clang-check -syntax-tree-dump %s -- -std=c++17 | FileCheck %s

// The range of a function defaulted or deleted in its class ends at the last
// character of the keyword; the syntax tree builder used to assert on it.
struct S {
  S() = default;
  S(const S &) = delete;
};
// CHECK: |-'='
// CHECK-NEXT: |-'default'
// CHECK-NEXT: `-';'
// CHECK: |-'='
// CHECK-NEXT: |-'delete'
// CHECK-NEXT: `-';'
