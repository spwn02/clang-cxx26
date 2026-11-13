// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -emit-pch -x c++-header %S/Inputs/token-sequence-diagnostic-header.h -o %t.pch
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -include-pch %t.pch -fsyntax-only -verify %s
// RUN: not %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -include-pch %t.pch -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=DIAG
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -include %S/Inputs/token-sequence-diagnostic-header.h -fsyntax-only -verify %s
// RUN: not %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -include %S/Inputs/token-sequence-diagnostic-header.h -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=DIAG
// RUN: %clang_cc1 -std=c++29 -DAST_ONLY -I %S/../Reflection/Inputs -include-pch %t.pch -ast-dump -ast-dump-filter=invoke_good %s 2>/dev/null | FileCheck %s --check-prefix=AST
// RUN: %clang_cc1 -std=c++29 -DAST_ONLY -I %S/../Reflection/Inputs -I %S/Inputs -include %S/Inputs/token-sequence-diagnostic-header.h -ast-dump -ast-dump-filter=invoke_good %s 2>/dev/null | FileCheck %s --check-prefix=AST

// expected-error@Inputs/token-sequence-diagnostic-header.h:4 {{use of undeclared identifier 'very_long_missing_identifier'}}
// expected-note@Inputs/token-sequence-diagnostic-header.h:4 {{in expansion of expression macro 'bad'}}

#ifndef AST_ONLY
int use() { return invoke(1); } // expected-note {{in instantiation of function template specialization 'invoke<int>' requested here}}
#endif
int good_use() { return invoke_good(1); }

// DIAG: token-sequence-diagnostic-header.h:4:49: error: use of undeclared identifier 'very_long_missing_identifier'
// DIAG: token-sequence-diagnostic-header.h:3:48: note: expanded from macro 'bad'
// DIAG: token-sequence-diagnostic-locations.cpp:13:20: note: in instantiation of function template specialization 'invoke<int>' requested here

// The instantiated macro invocation has the same source ranges with and
// without the PCH.
// AST: FunctionTemplateDecl {{.*}}token-sequence-diagnostic-header.h:6:1, col:68
// AST: CXXMacroInvocationExpr {{.*}}<col:54, col:65>
// AST: FunctionDecl {{.*}}<col:20, col:68> col:24 used invoke_good
