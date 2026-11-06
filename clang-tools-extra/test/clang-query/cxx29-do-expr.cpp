// RUN: clang-query -c "match doExpr()" %s -- -std=c++29 | FileCheck --check-prefix=EXPR %s
// RUN: clang-query -c "match doReturnStmt()" %s -- -std=c++29 | FileCheck --check-prefix=RET %s
// RUN: clang-query -c "match doReturnStmt(hasDoReturnValue(binaryOperator()))" %s -- -std=c++29 | FileCheck --check-prefix=VALUE %s
// RUN: clang-query -c "match doExpr(hasExplicitResultType())" %s -- -std=c++29 | FileCheck --check-prefix=EXPLICIT %s

int f(int a, int b) { return do { do_return a + b; }; }
int g() { return do -> int { do_return 1; }; }

// The same matchers are available through the dynamic registry (clang-query):
// two do expressions, two do_return statements, one of them with a binary operator
// operand, one do expression that spells its result type.
// EXPR: 2 matches.
// RET: 2 matches.
// VALUE: 1 match.
// EXPLICIT: 1 match.
