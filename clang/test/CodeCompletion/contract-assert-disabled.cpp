// Companion to contract-assert.cpp: confirms contract_assert is correctly
// absent from statement completion in C++23 (contracts are only part of C++26).

void withoutContracts() {
  // RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:6:5 -std=c++23 %s -o - | FileCheck %s
  co
  // CHECK: COMPLETION: Pattern : co_await <#expression#>
  // CHECK-NOT: contract_assert
}
