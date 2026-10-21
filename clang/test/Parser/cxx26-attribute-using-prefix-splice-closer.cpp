// RUN: %clang_cc1 -std=c++26 -freflection -freflection-latest -fsyntax-only -verify %s

// [diff.cpp23.dcl.dcl]: ':]' is a single token in C++26, so an
// attribute-using-prefix with no attributes and no whitespace is ill-formed.
struct [[using CC:]] C; // expected-error {{'[[using CC:]]' without whitespace before ']' is ill-formed}}
struct [[using DD: ]] D;
struct [[using gnu: unused]] E;
[[using clang: ]] extern int n;
