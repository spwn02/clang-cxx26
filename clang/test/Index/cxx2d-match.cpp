// RUN: c-index-test -test-load-source all -std=c++2d %s | FileCheck %s

// The cursors below a pattern matching expression: the subject, then for every case
// its pattern (the expressions, types and declarations written in it), guard and
// handler, in source order; a pattern variable's initializer is made up from the
// subject and is not visited under the variable.

struct P { int a, b; };
int f(P p, int x) {
  if (case [0, auto b] = p) return b;
  return match (x) {
    case 0 => 10;
    case 1 || 2 => 20;
    case auto y if (y > 5) => y;
    case _ => -1;
  };
}
bool g(int x) { return match(x, case 1 || 2); }

// CHECK: cxx2d-match.cpp:8:8: StructDecl=P:8:8 (Definition) Extent=[8:1 - 8:23]
// CHECK: cxx2d-match.cpp:8:16: FieldDecl=a:8:16 (Definition) Extent=[8:12 - 8:17] [access=public]
// CHECK: cxx2d-match.cpp:8:19: FieldDecl=b:8:19 (Definition) Extent=[8:12 - 8:20] [access=public]
// CHECK: cxx2d-match.cpp:9:5: FunctionDecl=f:9:5 (Definition) Extent=[9:1 - 17:2]
// CHECK: cxx2d-match.cpp:9:9: ParmDecl=p:9:9 (Definition) Extent=[9:7 - 9:10]
// CHECK: cxx2d-match.cpp:9:7: TypeRef=struct P:8:8 Extent=[9:7 - 9:8]
// CHECK: cxx2d-match.cpp:9:16: ParmDecl=x:9:16 (Definition) Extent=[9:12 - 9:17]
// CHECK: cxx2d-match.cpp:9:19: CompoundStmt= Extent=[9:19 - 17:2]
// CHECK: cxx2d-match.cpp:10:3: IfStmt= Extent=[10:3 - 10:37]
// CHECK: cxx2d-match.cpp:10:7: CaseConditionExpr= Extent=[10:7 - 10:27]
// CHECK: cxx2d-match.cpp:10:13: IntegerLiteral= Extent=[10:13 - 10:14]
// CHECK: cxx2d-match.cpp:10:21: VarDecl=b:10:21 (Definition) Extent=[10:16 - 10:22]
// CHECK: cxx2d-match.cpp:10:26: DeclRefExpr=p:9:9 Extent=[10:26 - 10:27]
// CHECK: cxx2d-match.cpp:10:29: ReturnStmt= Extent=[10:29 - 10:37]
// CHECK: cxx2d-match.cpp:10:36: UnexposedExpr=b:10:21 Extent=[10:36 - 10:37]
// CHECK: cxx2d-match.cpp:10:36: UnexposedExpr=b:10:21 Extent=[10:36 - 10:37]
// CHECK: cxx2d-match.cpp:10:36: DeclRefExpr=b:10:21 Extent=[10:36 - 10:37]
// CHECK: cxx2d-match.cpp:11:3: ReturnStmt= Extent=[11:3 - 16:4]
// CHECK: cxx2d-match.cpp:11:10: MatchSelectExpr= Extent=[11:10 - 16:4]
// CHECK: cxx2d-match.cpp:11:17: DeclRefExpr=x:9:16 Extent=[11:17 - 11:18]
// CHECK: cxx2d-match.cpp:12:10: IntegerLiteral= Extent=[12:10 - 12:11]
// CHECK: cxx2d-match.cpp:12:15: IntegerLiteral= Extent=[12:15 - 12:17]
// CHECK: cxx2d-match.cpp:13:10: IntegerLiteral= Extent=[13:10 - 13:11]
// CHECK: cxx2d-match.cpp:13:15: IntegerLiteral= Extent=[13:15 - 13:16]
// CHECK: cxx2d-match.cpp:13:20: IntegerLiteral= Extent=[13:20 - 13:22]
// CHECK: cxx2d-match.cpp:14:15: VarDecl=y:14:15 (Definition) Extent=[14:10 - 14:16]
// CHECK: cxx2d-match.cpp:14:21: BinaryOperator=> Extent=[14:21 - 14:26]
// CHECK: cxx2d-match.cpp:14:21: UnexposedExpr=y:14:15 Extent=[14:21 - 14:22]
// CHECK: cxx2d-match.cpp:14:21: DeclRefExpr=y:14:15 Extent=[14:21 - 14:22]
// CHECK: cxx2d-match.cpp:14:25: IntegerLiteral= Extent=[14:25 - 14:26]
// CHECK: cxx2d-match.cpp:14:31: UnexposedExpr=y:14:15 Extent=[14:31 - 14:32]
// CHECK: cxx2d-match.cpp:14:31: UnexposedExpr=y:14:15 Extent=[14:31 - 14:32]
// CHECK: cxx2d-match.cpp:14:31: DeclRefExpr=y:14:15 Extent=[14:31 - 14:32]
// CHECK: cxx2d-match.cpp:15:15: UnaryOperator= Extent=[15:15 - 15:17]
// CHECK: cxx2d-match.cpp:15:16: IntegerLiteral= Extent=[15:16 - 15:17]
// CHECK: cxx2d-match.cpp:18:6: FunctionDecl=g:18:6 (Definition) Extent=[18:1 - 18:48]
// CHECK: cxx2d-match.cpp:18:12: ParmDecl=x:18:12 (Definition) Extent=[18:8 - 18:13]
// CHECK: cxx2d-match.cpp:18:15: CompoundStmt= Extent=[18:15 - 18:48]
// CHECK: cxx2d-match.cpp:18:17: ReturnStmt= Extent=[18:17 - 18:45]
// CHECK: cxx2d-match.cpp:18:24: UnexposedExpr= Extent=[18:24 - 18:45]
// CHECK: cxx2d-match.cpp:18:24: MatchTestExpr= Extent=[18:24 - 18:45]
// CHECK: cxx2d-match.cpp:18:30: DeclRefExpr=x:18:12 Extent=[18:30 - 18:31]
// CHECK: cxx2d-match.cpp:18:38: IntegerLiteral= Extent=[18:38 - 18:39]
// CHECK: cxx2d-match.cpp:18:43: IntegerLiteral= Extent=[18:43 - 18:44]
