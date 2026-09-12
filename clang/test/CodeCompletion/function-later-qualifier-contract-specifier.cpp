// Completion at the end of an unrecognized partial identifier must reach the
// current function suffix parser, rather than function-definition recovery.

#if defined(CV)
void cv() const vo
// RUN: %clang_cc1 -DCV -fsyntax-only -code-completion-at=%s:5:19 -std=c++20 %s -o - | FileCheck %s --check-prefix=CV
// CV: COMPLETION: volatile

#elif defined(CONTRACT)
void contract() pre(true) po
// RUN: %clang_cc1 -DCONTRACT -fsyntax-only -fcontracts -code-completion-patterns -code-completion-at=%s:10:29 -std=c++26 %s -o - | FileCheck %s --check-prefix=CONTRACT
// CONTRACT: COMPLETION: Pattern : post(<#expression#>)
#endif
