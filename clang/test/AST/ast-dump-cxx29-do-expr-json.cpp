// RUN: %clang_cc1 -std=c++29 -ast-dump=json -ast-dump-filter=f %s | FileCheck %s --check-prefix=JSON
// RUN: %clang_cc1 -std=c++29 -ast-dump -ast-dump-filter=f %s | FileCheck %s --check-prefix=TEXT

int f(bool b) {
  return do [k = 1] -> int {
    if (b)
      do_return k;
    do_return 2;
  };
}

// JSON: "kind": "DoExpr"
// JSON: "hasExplicitType": true
// JSON: "hasInitStmt": true
// JSON: "kind": "DoReturnStmt"
// JSON: "kind": "DoReturnStmt"

// TEXT: DoExpr {{.*}} 'int'
// TEXT: DoReturnStmt
// TEXT: DoReturnStmt
