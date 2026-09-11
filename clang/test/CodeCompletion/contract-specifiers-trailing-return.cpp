// Regression test: the trailing-return-type position of a function
// declarator (`auto f() -> int <cursor>`) reaches code completion through a
// different path than the ordinary cv-qualifier position covered by
// contract-specifiers-qualifiers.cpp: ParseTrailingReturnType ->
// ParseTypeName -> ParseSpecifierQualifierList -> ParseDeclarationSpecifiers,
// whose own `case tok::code_completion:` (reached because
// DS.hasTypeSpecifier() is already true after consuming `int`) unwinds out
// of the entire declarator parse before CodeCompleteFunctionQualifiers or
// ParseContractSpecifierSequence are ever reached. Before this fix,
// SemaCodeCompletion::CodeCompleteDeclSpec (the handler for that switch
// case) offered only const/volatile, with no awareness that a function
// declarator -- cv-qualifiers, noexcept, and (with -fcontracts) pre/post --
// still follows.

auto freeFunction() -> int 

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:15:28 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: const
// CHECK-DAG: COMPLETION: noexcept
// CHECK-DAG: COMPLETION: Pattern : post(<#expression#>)
// CHECK-DAG: COMPLETION: Pattern : pre(<#expression#>)
// CHECK-DAG: COMPLETION: volatile
// CHECK-NOT: COMPLETION: final

// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:15:28 -std=c++26 %s -o - | FileCheck --check-prefix=DISABLED %s
// DISABLED-NOT: pre(<#expression#>)
// DISABLED-NOT: post(<#expression#>)
// DISABLED-DAG: COMPLETION: const
// DISABLED-DAG: COMPLETION: noexcept
