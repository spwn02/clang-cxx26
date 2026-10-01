// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// expected-no-diagnostics
int gv; const int cv = 0;
static_assert(__is_same(decltype([:^^gv:]), int));
static_assert(__is_same(decltype(([:^^gv:])), int&));
static_assert(__is_same(decltype([:^^cv:]), const int));
static_assert(__is_same(decltype(([:^^cv:])), const int&));

// [dcl.type.decltype] example.
const int&& foo();
decltype(foo()) x1 = 17;
decltype([:^^x1:]) x5 = 18;
decltype(([:^^x1:])) x6 = 19;
static_assert(__is_same(decltype(x5), const int&&));
static_assert(__is_same(decltype(x6), const int&));
struct S { int m; };
static_assert(__is_same(decltype([:^^S::m:]), int));
int& ref = gv;
static_assert(__is_same(decltype([:^^ref:]), int&));
void function();
static_assert(__is_same(decltype([:^^function:]), void()));
template<auto R> using designated_type = decltype([:R:]);
static_assert(__is_same(designated_type<^^gv>, int));
static_assert(__is_same(designated_type<^^ref>, int&));
