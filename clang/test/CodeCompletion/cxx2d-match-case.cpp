// Completion inside a match body (P2688): the start of a case, and a pattern.

int f(int x, int count) {
  return match (x) {
    
  };
}

int g(int x, int count) {
  return match (x) {
    case 
  };
}

// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-patterns -code-completion-at=%s:5:5 %s -o - | FileCheck -check-prefix=CHECK-CASE %s
// CHECK-CASE: COMPLETION: Pattern : case <#pattern#> => <#handler#>;
// CHECK-CASE: COMPLETION: Pattern : case _ => <#handler#>;
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-patterns -code-completion-at=%s:11:10 %s -o - | FileCheck -check-prefix=CHECK-PATTERN %s
// CHECK-PATTERN-DAG: COMPLETION: Pattern : auto <#name#>
// CHECK-PATTERN-DAG: COMPLETION: Pattern : _
// CHECK-PATTERN-DAG: COMPLETION: count : [#int#]count
// CHECK-PATTERN-DAG: COMPLETION: int
