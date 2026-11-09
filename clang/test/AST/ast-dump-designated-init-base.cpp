// RUN: %clang_cc1 -std=c++2d -ast-dump %s | FileCheck %s

struct A { int a1, a2; };
struct B : A { int b; };
B v = B{.a2 = 5, .b = 6};

// The base class subobject is initialized from the designators that name its
// members.
// CHECK:      VarDecl {{.*}} v 'B'
// CHECK:      InitListExpr {{.*}} 'B'
// CHECK-NEXT: |-InitListExpr {{.*}} 'A'
// CHECK-NEXT: | |-ImplicitValueInitExpr {{.*}} 'int'
// CHECK-NEXT: | `-IntegerLiteral {{.*}} 'int' 5
// CHECK-NEXT: `-IntegerLiteral {{.*}} 'int' 6
