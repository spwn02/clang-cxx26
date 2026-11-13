// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-pch -o %t.ast -x c++-header %S/Inputs/token-sequence-forms-header.h
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -x c++-header %S/Inputs/token-sequence-forms-header.h -ast-print > %t.direct.txt
// RUN: %clang_cc1 -std=c++2d -Wno-undefined-inline -ast-print %t.ast > %t.loaded.txt
// RUN: diff %t.direct.txt %t.loaded.txt
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -include-pch %t.ast -ast-dump -ast-dump-filter=_form %s | FileCheck %s
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -include-pch %t.ast -ast-dump -ast-dump-filter=statement_forms %s | FileCheck --check-prefix=STMT %s
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -include-pch %t.ast -fsyntax-only -verify %s
// expected-no-diagnostics

// Every form of macro invocation round-trips through a PCH: it is still an
// invocation around its expansion, and reads the same when printed.
static_assert(expression_form(1) == 2);
static_assert(instantiated() == 2);
int use(Holder h, int k) { return member_form(h) + (statement_forms(h, k), declared); }
static_assert(DeferredDeclaration<DeferredMacros>{}.added == 1);
static_assert(deferred_statement<DeferredMacros>(3) == 3);

// CHECK: FunctionDecl {{.*}} expression_form
// CHECK: CXXMacroInvocationExpr {{.*}} 'int'
// CHECK: FunctionDecl {{.*}} member_form
// CHECK: CXXMacroInvocationExpr {{.*}} 'int'
// STMT: FunctionDecl {{.*}} statement_forms
// STMT: CXXMacroInvocationExpr {{.*}} 'void'
// STMT: CXXMacroInvocationExpr {{.*}} 'int'
