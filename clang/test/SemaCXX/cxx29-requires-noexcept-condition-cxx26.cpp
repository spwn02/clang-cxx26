// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// P3822R2 is C++29 only; a plain 'noexcept' is unchanged.
void nothrows() noexcept;
static_assert(requires { { nothrows() } noexcept; });
static_assert(requires { { nothrows() } noexcept(true); }); // expected-error {{a condition for the 'noexcept' of a compound requirement is only available in C++29}}
