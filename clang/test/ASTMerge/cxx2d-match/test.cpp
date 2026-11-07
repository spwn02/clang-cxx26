// Pattern matching (P2688) is imported by the ASTImporter, also from two
// translation units that define the same entities, and as a dependent
// (template) expression.

// RUN: %clang_cc1 -std=c++2d -emit-pch -o %t.1.ast %S/Inputs/match1.cpp
// RUN: %clang_cc1 -std=c++2d -emit-pch -o %t.2.ast %S/Inputs/match2.cpp
// RUN: %clang_cc1 -std=c++2d -ast-merge %t.1.ast -ast-merge %t.2.ast -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-linux-gnu -ast-merge %t.1.ast -ast-merge %t.2.ast -emit-llvm -o - %s | FileCheck %s

// expected-no-diagnostics

int use(int x) { return select(x, P{0, 3}) + test(x) + dependent(P{1, 2}); }

// The imported functions are code generated like the ones written here.
// CHECK: define {{.*}} @_Z6selecti1P
// CHECK: icmp sgt i32 {{.*}}, 5
// CHECK: define {{.*}} @_Z4testi
// CHECK: define {{.*}} @_Z9dependentI1PEiT_
