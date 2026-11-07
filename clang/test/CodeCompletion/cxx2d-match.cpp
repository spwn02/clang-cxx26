// A statement can begin a match selection in C++29 and in no earlier mode.

void f(int x) {
  
}

// RUN: %clang_cc1 -std=c++2d -fsyntax-only -code-completion-patterns -code-completion-at=%s:4:3 %s -o - | FileCheck -check-prefix=CHECK-CXX29 %s
// CHECK-CXX29: COMPLETION: Pattern : match (<#subject#>) {
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -code-completion-patterns -code-completion-at=%s:4:3 %s -o - | FileCheck -check-prefix=CHECK-CXX26 %s
// CHECK-CXX26-NOT: Pattern : match
