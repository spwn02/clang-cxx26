// RUN: %clang_cc1 -std=c++23 -triple x86_64-linux-gnu -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++23 -triple x86_64-linux-gnu -include-pch %t -verify %s

#ifndef HEADER
#define HEADER
__float32 g32 = 1.5f32;
__float64 g64 = 2.5f64;
template <class T> constexpr T twice(T x) { return x + x; }
constexpr __float32 c32 = twice(1.0f32);
#else
static_assert(__is_same(decltype(g32), __float32));
static_assert(__is_same(decltype(g64), __float64));
static_assert(!__is_same(decltype(g32), float));
static_assert(c32 == 2.0f32);
static_assert(__is_same(decltype(twice(1.0f64)), __float64));
// expected-no-diagnostics
#endif
