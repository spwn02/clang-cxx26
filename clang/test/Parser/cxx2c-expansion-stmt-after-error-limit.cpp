// An expansion statement that comes after the limit of the errors (a fatal
// error): no instantiation context can be entered, and the statement is not
// expanded (Sema::SubstType asserted that it had a context).

// RUN: not %clang_cc1 -std=c++26 -ferror-limit 2 -fsyntax-only %s 2>&1 | FileCheck %s
// CHECK: too many errors emitted
// CHECK-NOT: Assertion

int a = u1;
int b = u2;
int c = u3;
void f() {
  template for (constexpr auto m : {1, 2}) { int v = m; }
}
