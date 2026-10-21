// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s
// expected-no-diagnostics

// [over.match.best.general]/2.4 (CWG1328): for direct binding of a reference
// to function type through a conversion function, the function whose return
// type is the same kind of reference as the one being initialized wins.
template <class T> struct A {
  operator T&();   // #1
  operator T&&();  // #2
};
typedef int Fn();
A<Fn> a;
Fn& lf = a;   // #1
Fn&& rf = a;  // #2

template <class T> struct B {
  operator T&();
  operator T&&();
};
int f();
B<Fn> b;
void use() {
  Fn& l = b;
  Fn&& r = b;
}

// Only one kind available: still fine.
struct OnlyL { operator Fn&(); };
struct OnlyR { operator Fn&&(); };
Fn& l1 = OnlyL();
Fn&& r1 = OnlyR();
