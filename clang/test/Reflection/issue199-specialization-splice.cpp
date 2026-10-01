// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
using T = [:^^int:]<int>; // expected-error {{reflection not usable in a template splice}}
using U = typename [:^^int:]<int>; // expected-error {{reflection not usable in a template splice}}
template<class> struct TT {};
using V = [:^^TT<int>:]<int>; // expected-error {{reflection not usable in a template splice}}
using W = [:^^TT:]<int>;
static_assert(__is_same(W, TT<int>));
template<auto R> using Dep = typename [:R:]<int>; // expected-error {{reflection not usable in a template splice}}
using Bad = Dep<^^int>; // expected-note {{in instantiation}}
