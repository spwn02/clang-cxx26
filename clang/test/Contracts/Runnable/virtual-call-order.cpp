// RUN: %clangxx -std=c++26 -fcontracts -freflection -fcontract-evaluation-semantic=quick_enforce %s -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 -fcontracts -freflection -fcontract-evaluation-semantic=quick_enforce -O2 %s -o %t
// RUN: %t | FileCheck %s

// RUN: %clangxx -std=c++26 -fcontracts -freflection -fcontract-evaluation-semantic=ignore %s -o %t
// RUN: %t | FileCheck %s --allow-empty --check-prefix=IGNORE
// IGNORE-NOT: {{[a-h]}}

extern "C" int putchar(int);
extern "C" int puts(const char *);

// [expr.call]: "the precondition assertions ... of the statically chosen
// function" precede "the precondition assertions of the function actually
// called"; postconditions reverse those groups.
// Draft example, verbatim (illustrative a-h are not declared in the draft):
/*
struct X1 { virtual void f()         pre(a) post(b) {} };
struct X2 { virtual void f()         pre(c) post(d) {} };
struct Y : X1    { void f() override pre(e) post(f) {} };
struct Z : Y, X2 { void f() override pre(g) post(h) {} };

void t() {
  Z z;
  z.f();                        // asserts g, h
  static_cast<Y*>(&z)->f();     // asserts e, g, h, f

  X1& x1ref = z;
  X2& x2ref = z;
  x1ref.f();                    // asserts a, g, h, b
  x2ref.f();                    // asserts c, g, h, d
  x1ref.X1::f();                // asserts a, b
  x1ref.[:^^X1::f:]();          // asserts a, g, h, b

  void (X1::*pmf)() = &X1::f;
  (x1ref.*pmf)();               // asserts g, h
}
*/
// Supply observable predicates and rename f to avoid the illustrative post(f)
// naming the member function instead of a predicate.
struct X1 { virtual void fun() pre((putchar('a'), true)) post((putchar('b'), true)) {} };
struct X2 { virtual void fun() pre((putchar('c'), true)) post((putchar('d'), true)) {} };
struct Y : X1 { void fun() override pre((putchar('e'), true)) post((putchar('f'), true)) {} };
struct Z : Y, X2 { void fun() override pre((putchar('g'), true)) post((putchar('h'), true)) {} };

int main() {
  Z z;
  z.fun(); puts("");
  static_cast<Y*>(&z)->fun(); puts("");
  X1& x1ref = z;
  X2& x2ref = z;
  x1ref.fun(); puts("");
  x2ref.fun(); puts("");
  x1ref.X1::fun(); puts("");
  x1ref.[:^^X1::fun:](); puts("");
  void (X1::*pmf)() = &X1::fun;
  (x1ref.*pmf)(); puts("");
}
// CHECK: gh
// CHECK-NEXT: eghf
// CHECK-NEXT: aghb
// CHECK-NEXT: cghd
// CHECK-NEXT: ab
// CHECK-NEXT: aghb
// CHECK-NEXT: gh
