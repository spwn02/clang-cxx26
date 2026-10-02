// RUN: %clang_cc1 -std=c++26 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -ast-merge %t -DTYPE_ORDER_PCH -fsyntax-only -verify %s
#ifndef TYPE_ORDER_PCH
#define TYPE_ORDER_PCH
struct Inc;
template<class T, class U> constexpr int order = __builtin_type_order(T, U);
constexpr int known = __builtin_type_order(int, char);
#else
// expected-no-diagnostics
static_assert(known != 0);
static_assert(order<Inc, int> != 0);
static_assert(order<int, char> == known);
static_assert(order<char, int> == -known);
static_assert(order<const int, int> != 0);
#endif
