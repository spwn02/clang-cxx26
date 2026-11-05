// RUN: %clang_cc1 -std=c++26 -ast-dump -ast-dump-filter=in_function %s | FileCheck %s

// A consteval block in a function body is a statement that ends where its
// body does (it used to end nowhere), and the dump shows its body.
int in_function() {
  consteval { }
  return 0;
}
// CHECK: FunctionDecl {{.*}} in_function
// CHECK-NOT: <invalid sloc>>
// CHECK: DeclStmt {{.*}} <line:[[@LINE-5]]:3, col:{{[0-9]+}}>
// CHECK-NEXT: ConstevalBlockDecl
// CHECK-NEXT: CXXOperatorCallExpr
