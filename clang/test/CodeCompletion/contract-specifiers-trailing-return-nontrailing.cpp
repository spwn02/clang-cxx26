// Companion to contract-specifiers-trailing-return.cpp: a regular (non-
// trailing-return-type) decl-specifier-seq completion position must NOT gain
// noexcept/pre/post as a side effect of this fix -- CodeCompleteDeclSpec's
// new IsTrailingReturnType parameter must default to false there, and
// `final` must still be offered exactly as before when the type-specifier
// being completed is class/struct.

struct T;
struct T 

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:9:10 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: final
// CHECK-NOT: COMPLETION: noexcept
// CHECK-NOT: pre(<#expression#>)
// CHECK-NOT: post(<#expression#>)
