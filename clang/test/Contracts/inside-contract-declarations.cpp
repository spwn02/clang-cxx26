// RUN: %clang_cc1 -std=c++26 -fcontracts -freflection -verify %s
// expected-no-diagnostics

// [expr.prim.id.unqual]: only entities "declared outside of C" are const in the
// predicate of C. Entities declared inside C, including those of lambdas
// written in C, keep their ordinary types.
void f()
    pre([] { static int s = 0; thread_local int t = 0; return ++s + ++t; }())
    pre([] { int a[1]{}; auto &[x] = a; return ++x; }())
    pre([] { int m = 0; return ++[:^^m:]; }())
    pre([k = 0]() mutable { return ++k; }())
    pre([] { static int s = 0; return ([]<int &R>() { return ++R; }).template operator()<s>(); }());
