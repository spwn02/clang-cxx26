// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -isystem %t/sys %t/user.cpp

// Function types with different language linkages are distinct types
// ([dcl.link]p1), but a use in a system header does not tell them apart, so the
// headers of an implementation written for compilers that ignore the language
// linkage of function types keep working.

//--- sys/h.h
extern "C" int cb(int);
int cxx_cb(int);

extern "C" { typedef int (*c_ptr)(int); }

template <class R> R call(R (*f)(int)) { return f(1); }

inline int in_system_header() {
  int (*p)(int) = cb;
  int (*q)(int) = &cb;
  c_ptr r = cxx_cb;
  return call(cb) + call(cxx_cb) + p(0) + q(0) + r(0);
}

//--- user.cpp
#include <h.h>

int use = in_system_header();

int (*p)(int) = cb; // expected-error {{cannot initialize a variable of type 'int (*)(int)' with an lvalue of type 'extern "C" int (int)'}}
int x = call(cb); // expected-error {{no matching function for call to 'call'}}
// expected-note@h.h:6 {{candidate template ignored: could not match 'R (int)' against 'extern "C" int (int)'}}
