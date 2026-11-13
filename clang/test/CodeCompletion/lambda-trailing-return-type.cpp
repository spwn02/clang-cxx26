// A completion inside the template argument list of a lambda's trailing return
// type completes a template argument (types), not a statement. The lookahead
// that checks for a template-id before '::' used to consume the code-completion
// token, which ran statement completion with the not yet typed call operator
// (an assertion in AddOrdinaryNameResults).
template <class T> struct Option {};
struct StringView {};
template <class F> struct S { template <class G> auto filterMap(G) { return 0; } };
void f(S<int> s) {
  s.filterMap([](int a) -> Option<
}
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -code-completion-at=%s:10:35 %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -code-completion-at=%s:10:36 %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -code-completion-at=%s:10:35 %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++29 -fsyntax-only -code-completion-at=%s:10:35 %s -o - | FileCheck %s
// CHECK-NOT: COMPLETION: Pattern : return
// CHECK: COMPLETION: StringView : StringView
