// Companion to contract-specifiers.cpp: `pre`/`post` are also valid directly
// after the declarator, before any cv-qualifier/noexcept/virt-specifier is
// written. That position is reached through the pre-existing
// SemaCodeCompletion::CodeCompleteFunctionQualifiers (already contracts-aware
// for this fix), not the new CodeCompleteFunctionContractSpecifiers entry
// point -- so `pre`/`post` appear here alongside const/volatile/noexcept
// rather than alone.

void freeFunction()

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:9:20 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: const
// CHECK-DAG: COMPLETION: noexcept
// CHECK-DAG: COMPLETION: Pattern : post(<#expression#>)
// CHECK-DAG: COMPLETION: Pattern : pre(<#expression#>)
// CHECK-DAG: COMPLETION: volatile

// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:9:20 -std=c++26 %s -o - | FileCheck --check-prefix=DISABLED %s
// DISABLED-NOT: pre(<#expression#>)
// DISABLED-NOT: post(<#expression#>)
