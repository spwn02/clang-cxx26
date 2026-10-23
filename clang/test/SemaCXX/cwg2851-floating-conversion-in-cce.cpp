// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// [expr.const]: a converted constant expression allows floating-point promotions and
// floating-point conversions where the source value can be represented exactly in the
// destination type (CWG2851).

template <float F> struct A {};
A<1.0> ok1;
A<0.5> ok2;
// expected-note@7 2{{template parameter is declared here}}
A<0.1> bad1;     // expected-error {{conversion from 'double' to 'float' is not allowed in a converted constant expression}}
A<1.0e100> bad2; // expected-error {{conversion from 'double' to 'float' is not allowed in a converted constant expression}}

template <double D> struct B {};
B<1.0f> ok3;
B<0.1f> ok4;

template <long double L> struct C {};
C<1.0> ok5;

constexpr float f = 0.25;
static_assert(f == 0.25f);
