// RUN: %clang_cc1 -std=c++26 -pedantic -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=none %s
// none-no-diagnostics

// [cpp.replace.general]: a translation unit shall not #undef a macro name
// lexically identical to a keyword.
#undef if // expected-warning {{undefining the macro 'if', a keyword, is ill-formed}}
#undef final // expected-warning {{undefining the macro 'final', a keyword, is ill-formed}}
#undef not_a_keyword
