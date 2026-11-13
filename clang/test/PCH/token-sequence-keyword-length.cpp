// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -emit-pch -x c++-header %S/Inputs/token-sequence-keyword-length.h -o %t.pch
// RUN: not %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -include-pch %t.pch -fsyntax-only -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s --check-prefix=DIAG
// RUN: not %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -I %S/Inputs -include %S/Inputs/token-sequence-keyword-length.h -fsyntax-only -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s --check-prefix=DIAG

S<Host> object;

// DIAG: token-sequence-keyword-length.h:8:14: error: expected ':'
// DIAG: fix-it:"{{.*}}token-sequence-keyword-length.h":{8:14-8:15}:":"
// DIAG: note: in member declarations injected into 'S<Host>'
// DIAG: token-sequence-keyword-length.cpp:5:9: note: in instantiation of template class 'S<Host>' requested here
