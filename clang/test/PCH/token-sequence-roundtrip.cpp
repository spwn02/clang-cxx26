// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -emit-pch -o %t.pch -x c++-header %S/Inputs/token-sequence-header.h
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -Wno-undefined-inline -include-pch %t.pch -fsyntax-only -verify %s

// Token sequences keep their tokens through a PCH: spelling, kind, and
// identity (a literal token, a keyword, an identifier and a punctuator).
// expected-no-diagnostics

static_assert(global_ts == ^^{ a + b * 3 });
static_assert(!(global_ts == ^^{ a + b * 4 }));
static_assert(empty_ts == ^^{ });
static_assert(make() == ^^{ int x = 1; "text"; 'c'; 1.5f; });
static_assert(joined() == ^^{ int x = 1; "text"; 'c'; 1.5f; ; return x; });

static_assert(with_type == ^^{ \(^^int) v });
static_assert(!(with_type == ^^{ \(^^long) v }));
static_assert(with_template == ^^{ \(^^Box)<int> b });
static_assert(with_value == ^^{ x = \(42) });
static_assert(with_namespace == ^^{ \(^^ns)::q });
