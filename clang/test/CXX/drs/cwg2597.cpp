// RUN: %clang_cc1 -std=c++20 -pedantic-errors -triple x86_64-unknown-unknown -emit-module-interface -verify %s -o /dev/null
// RUN: %clang_cc1 -std=c++23 -pedantic-errors -triple x86_64-unknown-unknown -emit-module-interface -verify %s -o /dev/null
// RUN: %clang_cc1 -std=c++2c -pedantic-errors -triple x86_64-unknown-unknown -emit-module-interface -verify %s -o /dev/null

// cwg2597: yes
// A replaceable global allocation or deallocation function declared in the
// purview of a module is attached to the global module, so it redeclares the
// implicit declaration.

// expected-no-diagnostics
export module M;

void *operator new(decltype(sizeof 0) n) { return __builtin_malloc(n); }
void operator delete(void *) noexcept {}
void f() { delete new int; }
