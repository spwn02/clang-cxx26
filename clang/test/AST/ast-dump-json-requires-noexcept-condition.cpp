// RUN: %clang_cc1 -std=c++2d -ast-dump=json %s | FileCheck %s

template <typename T, bool B>
concept C = requires(T t) { { t } noexcept; { t } noexcept(B); { t }; };

// Only the conditional requirement carries "noexceptCondition" (the condition
// is its last child expression); a plain noexcept does not.
// CHECK: "kind": "CompoundRequirement",
// CHECK: "noexcept": true,
// CHECK-NOT: "noexceptCondition"
// CHECK: "kind": "CompoundRequirement",
// CHECK: "noexcept": true,
// CHECK-NEXT: "noexceptCondition": true,
