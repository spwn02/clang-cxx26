// An atomic constraint of a requires-clause is a bool: a concept, a variable
// template of a trait, a requires-expression. Code completion of the
// constraint expression, in the trailing requires-clause of a function
// declarator as well as in the one after a template header, prefers the
// entities of that type (the preferred type is bool; clangd ranks by it, where
// it listed the members of a namespace in the alphabetical order of the names
// with the aliases and the unrelated variables first).

namespace N {
using alias_t = int;
int other;
struct Tag {};
template <class T> concept C = true;
template <class T> constexpr bool v = true;
}

#if CASE == 1
template <class T> struct S {
  void m()
    requires N::
  ;
};
#elif CASE == 2
template <class T>
void f(T) requires N::
;
#else
template <class T> requires N::
void g(T);
#endif

// RUN: %clang_cc1 -fsyntax-only -DCASE=1 -code-completion-at=%s:20:17 -std=c++26 %s -o - | FileCheck %s
// RUN: %clang_cc1 -fsyntax-only -DCASE=2 -code-completion-at=%s:25:23 -std=c++26 %s -o - | FileCheck %s
// RUN: %clang_cc1 -fsyntax-only -DCASE=3 -code-completion-at=%s:28:32 -std=c++26 %s -o - | FileCheck %s
// CHECK: PREFERRED-TYPE: _Bool
// CHECK: COMPLETION: C : C<<#class T#>>
// CHECK: COMPLETION: v : v<<#class T#>>
