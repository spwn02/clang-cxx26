// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// expected-no-diagnostics
struct S { int m; };
static_assert(sizeof([:^^S::m:]) == sizeof(int));
using T = decltype([:^^S::m:]);
void f(int = sizeof([:^^S::m:]));
template<auto R> constexpr auto member_size = sizeof([:R:]);
static_assert(member_size<^^S::m> == sizeof(int));
template<auto R> void dependent_default(int = sizeof([:R:]));
void call_default() { dependent_default<^^S::m>(); }
