// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 -pedantic-errors %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 -pedantic-errors %s
// cxx26-no-diagnostics

// P4271R0 (a defect report against C++26): ID_Compat_Math_Start and
// ID_Compat_Math_Continue characters are allowed in identifiers ([lex.name]).
constexpr int ∂ = 1;        // cxx23-error {{mathematical notation character <U+2202> in an identifier is a Clang extension}}
constexpr int x² = 2;       // cxx23-error {{mathematical notation character <U+00B2> in an identifier is a Clang extension}}
static_assert(∂ + x² == 3); // cxx23-error 2 {{mathematical notation character}}
