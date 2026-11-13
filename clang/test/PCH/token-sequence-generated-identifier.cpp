// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -emit-pch -x c++-header %S/Inputs/token-sequence-generated-identifier.h -o %t.pch
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -include-pch %t.pch -ast-dump-all -ast-dump-filter=Container %s | FileCheck %s --check-prefix=AST
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -include %S/Inputs/token-sequence-generated-identifier.h -ast-dump-all -ast-dump-filter=Container %s | FileCheck %s --check-prefix=AST

Container<Injector> object;

// AST: CXXMacroInvocationExpr {{.*}}<col:3, col:14> '<dependent type>'
// AST: FieldDecl {{.*}}<line:5:16, col:55> col:20 generated_identifier_with_length 'int'
