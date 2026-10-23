// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// [over.ics.ref]p1: binding a reference to a function type to a function that differs only by
// noexcept is a function pointer conversion, not an identity conversion (CWG2815).

struct R1 {};
struct R2 {};

void f() noexcept;
R1 g(void (*)() noexcept);
R2 g(void (&)());
static_assert(__is_same(decltype(g(f)), R1));

void g2() noexcept;
int h(void (&)() noexcept);
long h(void (&)());
static_assert(__is_same(decltype(h(g2)), int)); // exact match

void h2(void (&)());
void h2(void (&)() noexcept);
void call() { h2(g2); } // the noexcept overload is an exact match
// expected-no-diagnostics
