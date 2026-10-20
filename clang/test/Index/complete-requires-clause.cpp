// The atomic constraint of a requires-clause is a bool and is not a call: a
// concept (a concept-id is a prvalue of type bool) completes with the best
// priority, a variable template of type bool after it, the aliases, the
// other variables and the types after those; a function is not boosted, it
// would have to be called and a call is not a primary expression there.

namespace N {
using alias_t = int;
int other;
struct Tag {};
constexpr bool fn();
template <class T> concept C = true;
template <class T> constexpr bool v = true;
}

template <class T>
void f(T) requires N::
;

template <class T>
void g(T) requires (N::
;

// RUN: c-index-test -code-completion-at=%s:17:23 -std=c++26 %s | FileCheck %s
// A parenthesized primary is an ordinary expression: a function is boosted there.
// RUN: c-index-test -code-completion-at=%s:21:24 -std=c++26 %s | FileCheck --check-prefix=PAREN %s
// CHECK: TypeAliasDecl:{TypedText alias_t} (25)
// CHECK: ConceptDecl:{TypedText C}{LeftAngle <}{Placeholder class T}{RightAngle >} (3)
// CHECK: FunctionDecl:{ResultType bool}{TypedText fn}{LeftParen (}{RightParen )} (50)
// CHECK: VarDecl:{ResultType int}{TypedText other} (25)
// CHECK: StructDecl:{TypedText Tag} (50)
// CHECK: NotImplemented:{TypedText v}{LeftAngle <}{Placeholder class T}{RightAngle >} (12)
// PAREN: FunctionDecl:{ResultType bool}{TypedText fn}{LeftParen (}{RightParen )} (12)
