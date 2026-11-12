// A completion inside the trailing return type of a lambda used to dereference
// the null type of the lambda's call operator (assertion in
// AddOrdinaryNameResults when the return type was looked up for 'return').
template <class T> struct Option {};
template <class F> struct S { template <class G> auto filterMap(G) { return 0; } };
void f(S<int> s) {
  s.filterMap([](int a) -> Option<
}
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -code-completion-at=%s:7:35 %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -code-completion-at=%s:7:36 %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -code-completion-at=%s:7:35 %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++29 -fsyntax-only -code-completion-at=%s:7:35 %s -o - | FileCheck %s
// CHECK: COMPLETION: Pattern : return;
