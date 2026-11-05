// RUN: %clang_cc1 -std=c++26 -emit-pch -o %t.1.ast %S/Inputs/refl1.cpp
// RUN: %clang_cc1 -std=c++26 -ast-merge %t.1.ast -ast-print %s | FileCheck %s

// Reflection expressions (of a type, a member declaration, a namespace) are
// imported into another AST.

// CHECK: rt = ^^Rs;
// CHECK: rd = ^^Rs::m;
// CHECK: rn = ^^std::meta;
// CHECK: rb = ^^int;
