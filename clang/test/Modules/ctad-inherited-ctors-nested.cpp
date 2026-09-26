// Deduction guides from the inherited constructors of a member template of a
// class template, used across module boundaries (spwn02/clang-cxx26#122).
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++23 -emit-reduced-module-interface %t/lib.cppm -o %t/lib.pcm
// RUN: %clang_cc1 -std=c++23 -emit-reduced-module-interface %t/mid.cppm \
// RUN:   -fmodule-file=lib=%t/lib.pcm -o %t/mid.pcm
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %t/use.cpp \
// RUN:   -fmodule-file=lib=%t/lib.pcm -fmodule-file=mid=%t/mid.pcm
// RUN: %clang_cc1 -std=c++23 -emit-module-interface %t/lib.cppm -o %t/full-lib.pcm
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %t/use2.cpp \
// RUN:   -fmodule-file=lib=%t/full-lib.pcm

//--- lib.cppm
export module lib;
export template <typename T> struct Base {
  Base(T);
};
export template <typename T> struct Outer {
  template <typename U> struct Inner : Base<U> {
    using Base<U>::Base;
  };
};

//--- mid.cppm
export module mid;
import lib;
// Deduces (and so synthesizes the guides) inside another module.
export inline auto makeInMid() { return Outer<int>::Inner('c'); }

//--- use.cpp
// expected-no-diagnostics
import lib;
import mid;

Outer<int>::Inner a('x');
static_assert(__is_same(decltype(a), Outer<int>::Inner<char>));
Outer<long>::Inner b(1.5f);
static_assert(__is_same(decltype(b), Outer<long>::Inner<float>));
static_assert(__is_same(decltype(makeInMid()), Outer<int>::Inner<char>));

//--- use2.cpp
// expected-no-diagnostics
import lib;

Outer<int>::Inner a('x');
static_assert(__is_same(decltype(a), Outer<int>::Inner<char>));
