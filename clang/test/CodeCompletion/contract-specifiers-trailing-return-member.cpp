// Companion to contract-specifiers-trailing-return.cpp: the in-class member
// equivalent. Confirms noexcept newly appears here too (the fix lands in
// shared CodeCompleteDeclSpec infra reached from both free-function and
// member contexts) without duplicating pre/post -- ParseCXXMemberDeclaratorBeforeInitializer's
// own unconditional ParseContractSpecifierSequence call (which independently
// already offered pre/post here before this fix, via a second, separate
// completion pass) is now skipped specifically when the declarator has a
// trailing return type and completion was already handled.

struct S {
  auto member() -> int 
};

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:11:24 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: const
// CHECK-DAG: COMPLETION: noexcept
// CHECK-DAG: COMPLETION: Pattern : post(<#expression#>)
// CHECK-DAG: COMPLETION: Pattern : pre(<#expression#>)
// CHECK-DAG: COMPLETION: volatile
