// RUN: %clang_cc1 -std=c++20 -emit-module-interface %s -o /dev/null -verify

export module M;

namespace cwg2491 { // cwg2491: yes
struct S;
typedef S S;
export typedef S S;
struct T {}; // #cwg2491-prev
typedef T T;
export struct T;
// expected-error@-1 {{cannot export redeclaration 'T' here since the previous declaration has module linkage}}
// expected-note@#cwg2491-prev {{previous declaration is here}}
} // namespace cwg2491
