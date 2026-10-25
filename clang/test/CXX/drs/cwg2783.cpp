// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file --leading-lines %s %t
// RUN: %clang_cc1 -std=c++20 -pedantic-errors -triple x86_64-unknown-unknown -I%t -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 -pedantic-errors -triple x86_64-unknown-unknown -fmodule-file=M=%t/M.pcm -verify %t/use.cpp
// RUN: %clang_cc1 -std=c++23 -pedantic-errors -triple x86_64-unknown-unknown -I%t -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++23 -pedantic-errors -triple x86_64-unknown-unknown -fmodule-file=M=%t/M.pcm -verify %t/use.cpp
// RUN: %clang_cc1 -std=c++2c -pedantic-errors -triple x86_64-unknown-unknown -I%t -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++2c -pedantic-errors -triple x86_64-unknown-unknown -fmodule-file=M=%t/M.pcm -verify %t/use.cpp

// cwg2783: yes
// A deduction guide in the global module fragment for a class template that
// the module exports is reachable from the importer.

//--- S.h
template<class T>
struct S {
  S(const T *);
  template<class U> S(U);
};
template<class T>
S(T *) -> S<void>;

//--- M.cppm
module;
#include "S.h"
export module M;
export using ::S;

//--- use.cpp
// expected-no-diagnostics
import M;
void f(int *p) {
  S s{p};
  S<void> &r = s;
  (void)r;
}
