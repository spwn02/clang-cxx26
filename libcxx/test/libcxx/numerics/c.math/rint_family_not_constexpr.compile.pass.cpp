//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// nearbyint, rint, lrint and llrint are not constexpr: P0533R9 excludes the functions that depend strongly on the
// rounding mode, and [constexpr.functions] forbids an implementation to declare any other standard library function
// constexpr. The positive controls (floor, ceil, trunc, round, lround, which the draft does make constexpr) show that
// the check below detects a function that can be evaluated.

#include <cmath>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

// A class template, so that a non-constexpr callee is not diagnosed where the operator is defined.
#define DEFINE_OP(name)                                                                                                \
  template <class T>                                                                                                   \
  struct name##_op {                                                                                                   \
    constexpr int operator()() const { return static_cast<int>(std::name(static_cast<T>(0.5))); }                      \
  }

template <class Op>
concept constant_evaluable = requires { typename std::integral_constant<int, (Op{}(), 0)>; };

DEFINE_OP(nearbyint);
DEFINE_OP(rint);
DEFINE_OP(lrint);
DEFINE_OP(llrint);
DEFINE_OP(floor);
DEFINE_OP(ceil);
DEFINE_OP(trunc);
DEFINE_OP(round);
DEFINE_OP(lround);

template <class T>
constexpr bool rint_family_is_not_constexpr =
    !constant_evaluable<nearbyint_op<T>> && !constant_evaluable<rint_op<T>> && !constant_evaluable<lrint_op<T>> &&
    !constant_evaluable<llrint_op<T>>;

template <class T>
constexpr bool controls_are_constexpr =
    constant_evaluable<floor_op<T>> && constant_evaluable<ceil_op<T>> && constant_evaluable<trunc_op<T>> &&
    constant_evaluable<round_op<T>> && constant_evaluable<lround_op<T>>;

static_assert(rint_family_is_not_constexpr<float>);
static_assert(rint_family_is_not_constexpr<double>);
static_assert(rint_family_is_not_constexpr<long double>);
static_assert(controls_are_constexpr<float>);
static_assert(controls_are_constexpr<double>);
static_assert(controls_are_constexpr<long double>);

#if TEST_STD_VER >= 23 // P0533R9
#  ifdef __STDCPP_FLOAT16_T__
static_assert(rint_family_is_not_constexpr<std::float16_t>);
#  endif
#  ifdef __STDCPP_FLOAT32_T__
static_assert(rint_family_is_not_constexpr<std::float32_t>);
#  endif
#  ifdef __STDCPP_FLOAT64_T__
static_assert(rint_family_is_not_constexpr<std::float64_t>);
#  endif
#  ifdef __STDCPP_BFLOAT16_T__
static_assert(rint_family_is_not_constexpr<std::bfloat16_t>);
#  endif
#endif
