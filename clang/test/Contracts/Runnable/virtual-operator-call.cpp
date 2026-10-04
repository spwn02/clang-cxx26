// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce %s -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -O2 %s -o %t
// RUN: %t | FileCheck %s

extern "C" int putchar(int);
extern "C" int puts(const char *);

// [expr.call]: preconditions of "the statically chosen function" precede those
// of the function actually called. [over.match.oper]: `x@y` is interpreted as
// a call of the member function, so the same rule holds for operator notation.
struct B {
  virtual bool operator()(int) pre((putchar('a'), true)) post((putchar('b'), true)) { return true; }
  virtual bool operator==(const B &) const pre((putchar('c'), true)) post((putchar('d'), true)) { return true; }
  virtual ~B() = default;
};
struct D : B {
  bool operator()(int) override pre((putchar('e'), true)) post((putchar('f'), true)) { return true; }
  bool operator==(const B &) const override pre((putchar('g'), true)) post((putchar('h'), true)) { return true; }
};

// Argument values reach the statically chosen function's assertions; the
// object is argument 0 of an operator call expression.
struct V {
  virtual int operator()(const int v) pre((putchar('p'), v == 7)) post(r: (putchar('q'), r == v + 1)) { return v + 1; }
  virtual bool operator!() const pre((putchar('n'), true)) { return true; }
  virtual ~V() = default;
};
struct W : V {
  int operator()(int v) override { return v + 1; }
  bool operator!() const override pre((putchar('m'), true)) { return false; }
};

int main() {
  D d;
  B &r = d;
  r(1); puts("");
  (void)(r == r); puts("");
  W w;
  V &v = w;
  (void)v(7); puts("");
  (void)!v; puts("");
}
// CHECK: aefb
// CHECK-NEXT: cghd
// CHECK-NEXT: pq
// CHECK-NEXT: nm
