// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -emit-reduced-module-interface %t/forms.cppm -o %t/forms.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -emit-module-interface %t/forms.cppm -o %t/forms_full.pcm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -fmodule-file=forms=%t/forms_full.pcm -ast-dump -ast-dump-filter=_form %t/use.cpp | FileCheck %t/forms.cppm
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -fmodule-file=forms=%t/forms.pcm -fsyntax-only -verify %t/use.cpp

// Every form of macro invocation in a function exported by a module is read
// back as an invocation around its expansion, and the function can be used.

//--- forms.cppm
module;
#include "token-sequence.h"
export module forms;

export __macro inc(int x) { return ^^{ (\(x) + 1) }; }
export struct Holder {
  int v;
  __macro bump(this Holder &self, int n) { return ^^{ (\(self).v += \(n)) }; }
};
export constexpr int expression_form(int k) { return inc!(k); }
export inline int member_form(Holder h) { return h.bump!(1); }
export inline void statement_form(int k) { inc!(k); }
export template <class T> constexpr T dependent_form(T t) { return inc!(t); }
export struct DeferredMacros {
  static __macro declare(std::meta::token_sequence name) {
    return ^^{ int \(name) = 1; };
  }
  static __macro statement(int n) { return ^^{ (void)\(n); }; }
};
export template <class H> struct DeferredDeclaration { H::declare!(added); };
export template <class H> constexpr int deferred_statement(int n) {
  H::statement!(n);
  return n;
}

// CHECK: FunctionDecl {{.*}} expression_form
// CHECK: CXXMacroInvocationExpr {{.*}} 'int'
// CHECK: FunctionDecl {{.*}} member_form
// CHECK: CXXMacroInvocationExpr {{.*}} 'int'
// CHECK: FunctionDecl {{.*}} statement_form
// CHECK: CXXMacroInvocationExpr {{.*}} 'void'

//--- use.cpp
// expected-no-diagnostics
#include "token-sequence.h"
import forms;
static_assert(expression_form(41) == 42);
static_assert(dependent_form(1) == 2);
static_assert(DeferredDeclaration<DeferredMacros>{}.added == 1);
static_assert(deferred_statement<DeferredMacros>(3) == 3);
int use(Holder h) { statement_form(1); return member_form(h); }
