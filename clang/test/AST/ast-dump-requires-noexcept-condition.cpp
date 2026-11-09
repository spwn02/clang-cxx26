// RUN: %clang_cc1 -std=c++2d -ast-dump %s | FileCheck %s

template <class T, bool B> concept C = requires(T t) { { t.f() } noexcept(B); };

// The condition is a child of the requirement, after the expression.
// CHECK:      CompoundRequirement {{.*}} noexcept dependent
// CHECK-NEXT: |-CallExpr
// CHECK:      `-DeclRefExpr {{.*}} 'bool' NonTypeTemplateParm {{.*}} 'B'
