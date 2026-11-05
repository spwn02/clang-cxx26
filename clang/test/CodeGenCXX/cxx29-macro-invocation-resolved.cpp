// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -ast-dump %s | FileCheck --check-prefix=AST %s

// A resolved invocation stays in the AST around its expansion (tools see the
// macro name) and is transparent to code generation, constant evaluation and
// value categories.
#include "token-sequence.h"

__macro twice(int x) { return ^^{ (\(x) * 2) }; }
__macro first(int *p) { return ^^{ (*\(p)) }; }

constexpr int c = twice!(21);
static_assert(c == 42);
static_assert(noexcept(twice!(1)));

int g;
int &lv() { return first!(&g); }   // an lvalue expansion stays an lvalue
int rv(int a) { return twice!(a); }
void st() { int a = 1; first!(&a) = 5; }

// AST: CXXMacroInvocationExpr {{.*}} 'int'
// AST: UnresolvedLookupExpr {{.*}} 'twice'
// CHECK-LABEL: define {{.*}} @_Z2lvv
// CHECK: ret ptr @g
// CHECK-LABEL: define {{.*}} @_Z2rvi
// CHECK: mul nsw i32
// CHECK-LABEL: define {{.*}} @_Z2stv
// CHECK: store i32 5
