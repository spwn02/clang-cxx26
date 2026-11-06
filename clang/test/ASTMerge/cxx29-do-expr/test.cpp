// Do expressions (P2806) are imported by the ASTImporter, also from two
// translation units that define the same entities, and as a dependent
// (template) expression.

// RUN: %clang_cc1 -std=c++29 -emit-pch -o %t.1.ast %S/Inputs/do1.cpp
// RUN: %clang_cc1 -std=c++29 -emit-pch -o %t.2.ast %S/Inputs/do2.cpp
// RUN: %clang_cc1 -std=c++29 -ast-merge %t.1.ast -ast-merge %t.2.ast -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -ast-merge %t.1.ast -ast-merge %t.2.ast -ast-dump -ast-dump-filter=other %s | FileCheck %s

// CHECK: DoExpr
// CHECK: DoReturnStmt

// expected-no-diagnostics
static_assert(sum4 == 6);
static_assert(twice(21) == 42);
static_assert(twice(2.5) == 5.0);
int use() { return pick(true) + (int)other(2); }
