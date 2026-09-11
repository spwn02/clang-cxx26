// Regression test for issue #94: an ordinary member function declarator (no
// trailing return type) offered pre/post *twice* at the cv-qualifier
// position. Root cause: ParseFunctionDeclarator's cv-qualifier-seq parse
// already calls SemaCodeCompletion::CodeCompleteFunctionQualifiers here,
// which itself appends AddContractSpecifierResults -- so pre/post are
// already part of the first completion pass' results. But
// ParseCXXMemberDeclaratorBeforeInitializer's own unconditional call to
// ParseContractSpecifierSequence used to only skip itself when the
// declarator had a trailing return type (contract-specifiers-trailing-
// return-member.cpp's case), not for this plain case, so it fired a second,
// redundant completion pass over the same position. Each keyword/pattern
// must now appear exactly once.

struct A { void f() };

// RUN: %clang_cc1 -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:14:21 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: const
// CHECK-DAG: COMPLETION: final
// CHECK-DAG: COMPLETION: noexcept
// CHECK-DAG: COMPLETION: override
// CHECK-DAG: COMPLETION: Pattern : post(<#expression#>)
// CHECK-DAG: COMPLETION: Pattern : pre(<#expression#>)
// CHECK-DAG: COMPLETION: volatile
// Once the (unordered) CHECK-DAG block above has matched every entry,
// nothing after that point should repeat pre/post -- this is what actually
// catches the regression (each individually-duplicated entry still
// satisfies every CHECK-DAG above even when it appears twice).
// CHECK-NOT: pre(<#expression#>)
// CHECK-NOT: post(<#expression#>)
