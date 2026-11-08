// RUN: %clang_cc1 -std=c++2d -fsyntax-only -Wno-unused-value -ast-dump=json %s | FileCheck %s

// The JSON dump covers the pattern matching nodes.

int select(int x) {
  return match (x) { case 0 => 1; case _ => 2; };
}
bool test(int x) { return match(x, case 1); }
int condition(int x) {
  if (case int y = x) return y;
  return 0;
}

// CHECK: "kind": "MatchSelectExpr"
// CHECK: "kind": "MatchTestExpr"
// CHECK: "kind": "CaseConditionExpr"
