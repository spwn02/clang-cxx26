// RUN: %clang_cc1 -std=c++2d -x c++-header %s -emit-pch -o %t.pch
// RUN: %clang_cc1 -std=c++2d -x c++ /dev/null -include-pch %t.pch -fsyntax-only
// RUN: %clang_cc1 -std=c++2d -x c++-header %s -emit-pch -fpch-instantiate-templates -o %t.pch
// RUN: %clang_cc1 -std=c++2d -x c++ /dev/null -include-pch %t.pch -fsyntax-only

template <class T, bool B> concept C = requires(T t) { { t.f() } noexcept(B); };
struct Th { void f(); };
struct No { void f() noexcept; };
static_assert(C<Th, false> && !C<Th, true> && C<No, true>);
