// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:14:1 %s | FileCheck -check-prefix=CHECK-DECL %s
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:17:25 %s | FileCheck -check-prefix=CHECK-CALL %s
// RUN: not %clang_cc1 -std=c++26 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:14:1 %s | FileCheck -check-prefix=CHECK-NOKW %s
#include "token-sequence.h"

__macro plus_one(int x) { return ^^{ (\(x) + 1) }; }
struct S {
  __macro bump(this S &self, int n) { return ^^{ (\(self) + \(n)) }; }
};

// A declaration: the keyword is offered, in C++29 only.
// CHECK-DECL: COMPLETION: __macro
// CHECK-NOKW-NOT: COMPLETION: __macro

// An expression macro is completed with its `!`: without it the call is an
// ordinary function call and is not expanded.
int use() { return plus_ }
// CHECK-CALL: COMPLETION: plus_one : [#{{.*}}#]plus_one!(<#int x#>)
