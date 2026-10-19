// RUN: %clang_cc1 -std=c++26 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -fsyntax-only -verify %s
#ifndef TYPE_ORDER_PCH
#define TYPE_ORDER_PCH
#include "../SemaCXX/Inputs/std-compare.h"
struct Inc;
constexpr int sign(std::strong_ordering o) { return o < 0 ? -1 : o > 0 ? 1 : 0; }
template<class T, class U> constexpr int order = sign(__builtin_type_order(T, U));
constexpr int known = sign(__builtin_type_order(int, char));
#else
// expected-no-diagnostics
static_assert(known != 0);
static_assert(order<Inc, int> != 0);
static_assert(order<int, char> == known);
static_assert(order<char, int> == -known);
static_assert(order<const int, int> != 0);
#endif
