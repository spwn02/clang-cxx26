// RUN: %clangxx -std=c++2d -fcontract-evaluation-semantic=quick_enforce %s -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++2d -fcontract-evaluation-semantic=quick_enforce -O2 %s -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++2d -fcontract-evaluation-semantic=ignore %s -o %t
// RUN: %t | FileCheck %s --check-prefix=IGNORE

// P3097R3 ([expr.call] in N5054): the example of the draft. The assertions of
// a call are those of the statically chosen function (only for a virtual call
// through a class member access) and those of the function actually called.
extern "C" int putchar(int);

static bool t(char c) {
  putchar(c);
  return true;
}
static void end(const char *) { putchar('\n'); }

struct X1 { virtual void f()         pre(t('a')) post(t('b')) {} };
struct X2 { virtual void f()         pre(t('c')) post(t('d')) {} };
struct Y : X1    { void f() override pre(t('e')) post(t('F')) {} };
struct Z : Y, X2 { void f() override pre(t('g')) post(t('h')) {} };

int main() {
  Z z;
  z.f();                        // g h
  end("");
  static_cast<Y *>(&z)->f();    // e g h F
  end("");

  X1 &x1ref = z;
  X2 &x2ref = z;
  x1ref.f();                    // a g h b
  end("");
  x2ref.f();                    // c g h d
  end("");
  x1ref.X1::f();                // a b
  end("");
  x1ref.[:^^X1::f:]();          // a g h b
  end("");

  void (X1::*pmf)() = &X1::f;
  (x1ref.*pmf)();               // g h
  end("");
}

// CHECK: gh
// CHECK-NEXT: eghF
// CHECK-NEXT: aghb
// CHECK-NEXT: cghd
// CHECK-NEXT: ab
// CHECK-NEXT: aghb
// CHECK-NEXT: gh
// IGNORE-NOT: {{[a-h]}}
