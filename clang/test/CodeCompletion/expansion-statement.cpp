// C++26 expansion statements (P1306R5, `template for`) had no code-completion
// support at all: `template` wasn't offered as a statement-position keyword,
// even though ordinary statement keywords (`for`, `while`, `if`, ...)
// complete correctly in the same position. Fixed by adding a `template for
// (<#range-declaration#> : <#range-expression#>) { <#statements#> }` code
// pattern to PCC_Statement's completion results, alongside the existing
// range-based `for` pattern it mirrors, gated on -fexpansion-statements
// exactly like the parser's own `getLangOpts().ExpansionStatements` check
// (see ParseStmt.cpp's `case tok::kw_template:`).

void f() {  }
void g() { te }

// RUN: %clang_cc1 -fsyntax-only -fexpansion-statements -code-completion-patterns -code-completion-at=%s:11:11 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: Pattern : for (<#init-statement#>; <#condition#>; <#inc-expression#>) {
// CHECK-DAG: COMPLETION: Pattern : template for (<#range-declaration#> : <#range-expression#>) {

// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:11:11 -std=c++26 %s -o - | FileCheck --check-prefix=DISABLED %s
// DISABLED-NOT: template for
// DISABLED-DAG: COMPLETION: Pattern : for (<#init-statement#>; <#condition#>; <#inc-expression#>) {

// A two-character prefix must filter down to exactly this pattern, proving
// this isn't just a fallback/unfiltered listing.
// RUN: %clang_cc1 -fsyntax-only -fexpansion-statements -code-completion-patterns -code-completion-at=%s:12:14 -std=c++26 %s -o - | FileCheck --check-prefix=PREFIX %s
// PREFIX: COMPLETION: Pattern : template for (<#range-declaration#> : <#range-expression#>) {
// PREFIX-NOT: COMPLETION:
