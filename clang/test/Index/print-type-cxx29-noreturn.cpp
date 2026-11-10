// RUN: c-index-test -test-print-type -std=c++2d -fcxx-exceptions %s | FileCheck %s

// P3549: the type of a throw-expression is the bottom type std::noreturn_t; it
// has its own CXTypeKind (CXType_NoReturn).
auto g() { throw 1; }
// CHECK: FunctionDecl=g:{{.*}} [type=std::noreturn_t ()]{{.*}} [resulttype=std::noreturn_t]
// CHECK: CXXThrowExpr= [type=std::noreturn_t] [typekind=NoReturn]
