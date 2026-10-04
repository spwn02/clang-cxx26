// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce %s -o %t
// RUN: %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -O2 %s -o %t
// RUN: %t

// [expr.call]: "the precondition assertions ... of the statically chosen
// function" precede those of "the function actually called"; the statically
// chosen function's postconditions follow those of the function actually called.
// The predicates must see the static subobject, initialized parameters, and
// the returned value (including covariant conversion).
struct Pad { virtual ~Pad() {} int padding = 42; };
struct Base {
  int tag = 7;
  virtual int f(const int n, int &out)
    pre(n == tag && out == 0)
    post(r: r == n + tag && out == n) { out = n; return n + tag; }
  virtual Base *self() post(r: r == this) { return this; }
};
struct Derived : Pad, Base {
  int f(const int n, int &out) override
    pre(n == tag && out == 0)
    post(r: r == n + tag && out == n) { out = n; return n + tag; }
  Derived *self() override post(r: r == this) { return this; }
};
int argument_evaluations;
int argument() { ++argument_evaluations; return 7; }
void test(Base *b) {
  int out = 0;
  if (b->f(argument(), out) != 14 || out != 7 || argument_evaluations != 1)
    __builtin_trap();
  if (b->self() != b)
    __builtin_trap();
}
struct Abstract {
  virtual int f(const int n) pre(n == 7) post(r: r == n) = 0;
};
struct Concrete : Abstract {
  int f(const int n) override { return n; }
};
int main() {
  Derived d;
  test(&d);
  Base b;
  argument_evaluations = 0;
  test(&b);
  Concrete c;
  Abstract *a = &c;
  if (a->f(7) != 7)
    __builtin_trap();
}
