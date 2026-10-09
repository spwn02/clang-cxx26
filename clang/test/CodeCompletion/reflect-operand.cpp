// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -code-completion-at=%s:15:26 %s -o - | FileCheck -check-prefix=CHECK-PREFIX %s
// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -code-completion-at=%s:16:24 %s -o - | FileCheck -check-prefix=CHECK-EMPTY %s
// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -code-completion-at=%s:17:42 %s -o - | FileCheck -check-prefix=CHECK-QUALIFIED %s

namespace evaluation_ns {
struct evaluator {};
int evalue;
} // namespace evaluation_ns
struct Evaluator {};
template <class T> struct evtemplate {};
int ev_global();

void f() {
  int ev_local = 0;
  constexpr auto a = ^^ev_global;
  constexpr auto b = ^^int;
  constexpr auto c = ^^evaluation_ns::evaluator;
}

// A name that can be reflected is offered: variables, namespaces and the
// other names of the scope; not only the names of the recovery completion.
// CHECK-PREFIX-DAG: COMPLETION: ev_global : [#int#]ev_global()
// CHECK-PREFIX-DAG: COMPLETION: ev_local : [#int#]ev_local
// CHECK-PREFIX-DAG: COMPLETION: evaluation_ns : evaluation_ns::
// CHECK-PREFIX-DAG: COMPLETION: evtemplate : evtemplate<<#class T#>>
// CHECK-EMPTY-DAG: COMPLETION: int
// CHECK-EMPTY-DAG: COMPLETION: Evaluator : Evaluator
// CHECK-EMPTY-DAG: COMPLETION: evaluation_ns : evaluation_ns::
// CHECK-QUALIFIED-DAG: COMPLETION: evaluator : evaluator
// CHECK-QUALIFIED-DAG: COMPLETION: evalue : [#int#]evalue
