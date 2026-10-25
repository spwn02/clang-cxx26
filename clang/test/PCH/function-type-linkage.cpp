// RUN: %clang_cc1 -std=c++17 -emit-pch -o %t -DBUILD %s
// RUN: %clang_cc1 -std=c++17 -include-pch %t -DUSE -verify %s

// The language linkage of a function type is part of the type and survives the
// round trip through a precompiled header ([dcl.link]).
#ifdef BUILD
extern "C" typedef void CFn(int);
typedef void CxxFn(int);
extern "C" void cf(int);
void xf(int);
#endif

#ifdef USE
CFn *a = cf;
CxxFn *b = xf;
CFn *c = xf;
// expected-error@-1 {{cannot initialize a variable of type 'CFn *' (aka 'extern "C" void (*)(int)') with an lvalue of type 'void (int)'}}
CxxFn *d = cf;
// expected-error@-1 {{cannot initialize a variable of type 'CxxFn *' (aka 'void (*)(int)') with an lvalue of type 'extern "C" void (int)'}}
#endif
