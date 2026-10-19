// A class template that a precompiled header and a module (that includes the
// same header in its global module fragment) both specialize: the type of the
// specialization is one type.
//
// The preferred_name attribute of the template (as libc++'s basic_string_view)
// names a typedef of the specialization. Reading the attribute while the
// specialization of the header was read created the type of the class before
// it was merged with the specialization of the module (that was read first),
// and the typedef of the header and the one of the module were two types.

// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -I%t -emit-module-interface %t/m.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 -I%t -emit-pch -x c++-header %t/pre.h -o %t/pre.pch
// RUN: %clang_cc1 -std=c++20 -fmodule-file=M=%t/M.pcm -include-pch %t/pre.pch \
// RUN:     -fsyntax-only -verify %t/b.cpp

//--- h.h
#pragma once
inline namespace v1 {
template <class T> struct Tr {};
template <class T, class U = Tr<T>> class S;
using SC = S<char>;
using SW = S<wchar_t>;
template <class T, class U>
class [[clang::preferred_name(SC)]] [[clang::preferred_name(SW)]] S;
template <class T, class U> class S {
public:
  T v;
};
void g(SC);
inline void use_header() { S<char> s; g(s); }
} // namespace v1

//--- pre.h
#include "h.h"

//--- m.cppm
module;
#include "h.h"
export module M;
export using ::S;
export using ::SC;
export inline void m(SC) {}
export inline void use_module() { S<char> s; m(s); }

//--- b.cpp
// expected-no-diagnostics
import M;
template <class F> struct P;
template <class A> struct P<void (*)(A)> { using type = A; };
// m is read from the module, g from the header.
using MT = P<decltype(&m)>::type;
using GT = P<decltype(&g)>::type;
static_assert(__is_same(MT, GT));
void t() {
  SC c;
  m(c);
  g(c);
  static_assert(__is_same(SC, S<char>));
}
