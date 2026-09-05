// Regression test for the Contracts Hardening epic's M6
// (docs/CONTRACTS_HARDENING.md): clangd never suggested `contract_assert` as
// a statement completion with -fcontracts enabled, because
// SemaCodeComplete.cpp had zero contracts-aware completion results (unlike
// static_assert/co_return, gated the same way on their own language
// options). Root cause was a completion gap, not a lexing/parsing one --
// contract_assert was never flagged as an error, it just never appeared in
// the suggestion list. See contract-assert-disabled.cpp for the -fcontracts
// absent counterpart (kept in a separate file so each file has exactly one
// completion trigger and no other partial statement to misparse first).

void withContracts() {
  // RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:14:5 -std=c++26 %s -o - | FileCheck %s
  co
  // CHECK: COMPLETION: Pattern : co_await <#expression#>
  // CHECK-NEXT: COMPLETION: Pattern : co_yield <#expression#>
  // CHECK-NEXT: COMPLETION: const
  // CHECK-NEXT: COMPLETION: Pattern : const_cast<<#type#>>(<#expression#>)
  // CHECK-NEXT: COMPLETION: constexpr
  // CHECK-NEXT: COMPLETION: constinit
  // CHECK-NEXT: COMPLETION: Pattern : contract_assert(<#expression#>);
}
