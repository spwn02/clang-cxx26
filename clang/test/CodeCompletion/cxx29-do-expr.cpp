// Code completion for do-expressions (P2806): the `do` keyword in an
// expression, what may follow `do`, `do_return` only where it is legal, and the
// preferred type of the `do_return` operand.

int use(int n) {
  int a =  0;
  int b = do  { do_return 1; };
  int c = do [k = 1]  { do_return k; };
  int e = do -> int {  do_return 0; };
  int g = do { if (n) {  } do_return 1; };
  int i = do { auto l = [] {  }; do_return 1; };
  (void)do -> void {  };
  int m = do { do_return 1; } +  1;
  return n;
}

void outside() {
  
}

void operand() {
  long lv = 0;
  int iv = 0;
  long r = do -> long { do_return  lv; };
  int s = do { do_return  iv; };
}

// A do-expression is offered in an expression.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:6:11 %s | FileCheck -check-prefix=CHECK-EXPR %s
// CHECK-EXPR: COMPLETION: Pattern : do {
// CHECK-EXPR-NEXT: <#statements#>
// CHECK-EXPR-NEXT: }

// Right after `do`: the init-captures, the result type, the body.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:7:14 %s | FileCheck -check-prefix=CHECK-INTRO %s
// CHECK-INTRO-DAG: COMPLETION: Pattern : [<#name = init#>]
// CHECK-INTRO-DAG: COMPLETION: Pattern : -> <#type#>
// CHECK-INTRO-DAG: COMPLETION: Pattern : {
// CHECK-INTRO-NOT: do_return

// After the init-captures there is no second capture list.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:8:22 %s | FileCheck -check-prefix=CHECK-AFTER-CAPTURES %s
// CHECK-AFTER-CAPTURES-NOT: COMPLETION: Pattern : [<#name = init#>]
// CHECK-AFTER-CAPTURES-DAG: COMPLETION: Pattern : -> <#type#>
// CHECK-AFTER-CAPTURES-DAG: COMPLETION: Pattern : {

// In the body `do_return` is offered in the forms the result type allows: an
// explicit non-void type needs an operand ...
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:9:23 %s | FileCheck -check-prefix=CHECK-BODY-INT %s
// CHECK-BODY-INT: COMPLETION: Pattern : do_return <#expression#>;
// CHECK-BODY-INT-NOT: COMPLETION: Pattern : do_return;
// ... a deduced one allows both, also in a nested block ...
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:10:25 %s | FileCheck -check-prefix=CHECK-DEDUCED %s
// CHECK-DEDUCED: COMPLETION: Pattern : do_return;
// CHECK-DEDUCED: COMPLETION: Pattern : do_return <#expression#>;
// ... and a void one only the bare form.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:12:22 %s | FileCheck -check-prefix=CHECK-VOID %s
// CHECK-VOID: COMPLETION: Pattern : do_return;
// CHECK-VOID-NOT: COMPLETION: Pattern : do_return <#expression#>;

// A lambda in the body is a different function: its `return` is its own and
// `do_return` is not available there. Neither is it outside a do-expression.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:11:30 %s | FileCheck -check-prefix=CHECK-NO-DO-RETURN %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:18:3 %s | FileCheck -check-prefix=CHECK-NO-DO-RETURN %s
// CHECK-NO-DO-RETURN-NOT: do_return

// A do-expression is offered again after an operator.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:13:33 %s | FileCheck -check-prefix=CHECK-EXPR %s

// The operand of `do_return` prefers the explicit result type; with a deduced
// type nothing is preferred (the two variables rank alike).
// RUN: c-index-test -code-completion-at=%s:24:35 -std=c++2d %s | FileCheck -check-prefix=CHECK-PREF-LONG %s
// CHECK-PREF-LONG-DAG: VarDecl:{ResultType int}{TypedText iv} ([[IV:[0-9]+]])
// CHECK-PREF-LONG-DAG: VarDecl:{ResultType long}{TypedText lv} ([[LV:[0-9]+]])
// RUN: c-index-test -code-completion-at=%s:25:26 -std=c++2d %s | FileCheck -check-prefix=CHECK-PREF-NONE %s
// CHECK-PREF-NONE: VarDecl:{ResultType int}{TypedText iv} ([[P:[0-9]+]])
// CHECK-PREF-NONE: VarDecl:{ResultType long}{TypedText lv} ([[P]])

// Where the feature is off there is no do-expression snippet; the flag brings it back.
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -code-completion-at=%s:6:11 %s | FileCheck -check-prefix=CHECK-OFF %s
// CHECK-OFF-NOT: COMPLETION: Pattern : do {
// RUN: %clang_cc1 -std=c++26 -fdo-expressions -fsyntax-only -code-completion-at=%s:6:11 %s | FileCheck -check-prefix=CHECK-EXPR %s

void resulttype() {
  int t = do -> int  { do_return 0; };
}

// After the result type the body `{` is offered along with the declarator
// continuations.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:85:21 %s | FileCheck -check-prefix=CHECK-AFTER-TYPE %s
// CHECK-AFTER-TYPE: COMPLETION: Pattern : {
// CHECK-AFTER-TYPE-NEXT: <#statements#>
// No function declarator follows the result type of a do expression.
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-at=%s:85:21 %s | FileCheck -check-prefix=CHECK-NO-DECLARATOR %s
// CHECK-NO-DECLARATOR-NOT: noexcept
// CHECK-NO-DECLARATOR-NOT: requires
// CHECK-NO-DECLARATOR-NOT: operator
// CHECK-NO-DECLARATOR-NOT: post(
// CHECK-NO-DECLARATOR-NOT: pre(
