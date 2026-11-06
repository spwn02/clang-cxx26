// RUN: c-index-test -test-load-source all -std=c++29 %s | FileCheck %s

int f(bool b) {
  return do [k = 1] -> int {
    if (b)
      do_return k;
    int local = k + 1;
    do_return local;
  };
}

// CHECK: cxx29-do-expr.cpp:3:5: FunctionDecl=f:3:5 (Definition) Extent=[3:1 - 10:2]
// CHECK: cxx29-do-expr.cpp:4:10: DoExpr= Extent=[4:10 - 9:4]
// CHECK: cxx29-do-expr.cpp:4:14: VarDecl=k:4:14 (Definition) Extent=[4:14 - 4:19]
// CHECK: cxx29-do-expr.cpp:5:5: IfStmt= Extent=[5:5 - 6:18]
// CHECK: cxx29-do-expr.cpp:6:7: DoReturnStmt= Extent=[6:7 - 6:18]
// CHECK: cxx29-do-expr.cpp:6:17: DeclRefExpr=k:4:14 Extent=[6:17 - 6:18]
// CHECK: cxx29-do-expr.cpp:7:9: VarDecl=local:7:9 (Definition) Extent=[7:5 - 7:22]
// CHECK: cxx29-do-expr.cpp:8:5: DoReturnStmt= Extent=[8:5 - 8:20]
// CHECK: cxx29-do-expr.cpp:8:15: DeclRefExpr=local:7:9 Extent=[8:15 - 8:20]
