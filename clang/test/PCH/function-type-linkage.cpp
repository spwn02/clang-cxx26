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
// expected-error@-1 {{converting 'void (*)(int)' to 'CFn *' (aka 'extern "C" void (*)(int)') changes the language linkage of the function type}}
CxxFn *d = cf;
// expected-error@-1 {{converting 'extern "C" void (*)(int)' to 'CxxFn *' (aka 'void (*)(int)') changes the language linkage of the function type}}
#endif
