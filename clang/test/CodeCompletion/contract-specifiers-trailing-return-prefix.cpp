// Companion to contract-specifiers-trailing-return.cpp: typing a partial
// contract keyword after a trailing return type must not silently drop to
// zero completions (the bug's second reported symptom, fixed by the same
// CodeCompleteDeclSpec change) -- filtered by the typed prefix "p", only
// pre/post should remain.

auto freeFunctionTypedPrefix() -> int p

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:7:40 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: Pattern : post(<#expression#>)
// CHECK-DAG: COMPLETION: Pattern : pre(<#expression#>)
// CHECK-NOT: COMPLETION: const
// CHECK-NOT: COMPLETION: volatile
// CHECK-NOT: COMPLETION: noexcept
