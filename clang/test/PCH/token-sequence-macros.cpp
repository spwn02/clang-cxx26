// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-pch -o %t.pch -x c++-header %S/Inputs/token-sequence-macros-header.h
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -include-pch %t.pch -fsyntax-only -verify %s
// expected-no-diagnostics

// What was defined in the PCH works in the file that includes it: a macro
// with an expression parameter and one with a raw token sequence parameter, a
// template whose literal names its parameter, a dependent invocation
// instantiated here, and an inline function holding a literal.
static_assert(inc!(41) == 42);
static_assert(keep!(6 * 7) == 42);
static_assert(one_more(1) == 2);
static_assert(addone(41) == 42);
static_assert(addone(2) == 3);
// (The substituted parameter is a type token, not the keyword: it equals
// itself and differs per argument.)
static_assert(declare<int>() == declare<int>());
static_assert(!(declare<int>() == declare<char>()));
static_assert(!(declare<int>() == ^^{ v }));
static_assert(shared() == ^^{ a b c });
