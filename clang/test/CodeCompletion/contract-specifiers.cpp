// Regression test: with -fcontracts, `pre`/`post` never appeared in clangd's
// completions even though `contract_assert` did (fixed separately, see
// contract-assert.cpp). Root cause was a position gap rather than a missing
// result: contract specifiers are parsed by ParseContractSpecifierSequence,
// which sits *after* the virt-specifiers, past the point
// SemaCodeCompletion::CodeCompleteFunctionQualifiers covers. A code-completion
// token is not a contract keyword, so that function's
// `if (!isFunctionContractKeyword(Tok)) return;` dropped the completion
// entirely and nothing was offered at all.
//
// This file covers the contract-specifier position itself, which reaches the
// dedicated CodeCompleteFunctionContractSpecifiers entry point and so offers
// *only* `pre`/`post`. contract-specifiers-qualifiers.cpp covers the earlier
// cv-qualifier position, which reaches CodeCompleteFunctionQualifiers instead
// and offers them alongside const/volatile/noexcept.

struct S {
  void m() const noexcept
};

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:18:27 -std=c++26 %s -o - | FileCheck %s
// CHECK: COMPLETION: Pattern : post(<#expression#>)
// CHECK-NEXT: COMPLETION: Pattern : pre(<#expression#>)

// Without -fcontracts the parser must not consume the token here: the guard in
// ParseContractSpecifierSequence leaves it to the pre-existing declaration
// completion, so behavior for non-contracts translation units is unchanged.
// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:18:27 -std=c++26 %s -o - | FileCheck --check-prefix=DISABLED %s
// DISABLED-NOT: pre(<#expression#>)
// DISABLED-NOT: post(<#expression#>)
