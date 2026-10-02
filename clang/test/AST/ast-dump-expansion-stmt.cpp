// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -ast-dump %s | FileCheck %s
void enumerating() {
  template for (auto x : {1, 2}) { (void)x; }
}
// CHECK-LABEL: FunctionDecl {{.*}} enumerating
// CHECK: ExpansionStmtDecl
// CHECK: CXXInitListExpansionStmt
// CHECK: CompoundStmt
// CHECK: CStyleCastExpr
// CHECK: instantiation: CompoundStmt
// CHECK: VarDecl {{.*}} x 'int'
// CHECK: IntegerLiteral {{.*}} 1
// CHECK: CStyleCastExpr
// CHECK: instantiation: CompoundStmt
// CHECK: VarDecl {{.*}} x 'int'
// CHECK: IntegerLiteral {{.*}} 2
// CHECK: CStyleCastExpr

template <class T> void dependent(T range) {
  template for (auto x : range) { (void)x; }
}
// CHECK-LABEL: FunctionTemplateDecl {{.*}} dependent
// CHECK: ExpansionStmtDecl
// CHECK: CXXIndeterminateExpansionStmt
// CHECK: CompoundStmt
// CHECK: CStyleCastExpr
