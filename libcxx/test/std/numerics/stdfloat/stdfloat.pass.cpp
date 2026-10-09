// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <stdfloat>

#include <concepts>
#include <cassert>
#include <functional>
#include <limits>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

#if _LIBCPP_STD_VER >= 23
#  if defined(__STDCPP_FLOAT16_T__)
static_assert(!std::is_same_v<std::float16_t, float>);
static_assert(!std::is_same_v<std::float16_t, double>);
static_assert(!std::is_same_v<std::float16_t, long double>);
static_assert(sizeof(std::float16_t) == 2);
static_assert(std::is_floating_point_v<std::float16_t>);
static_assert(std::is_arithmetic_v<std::float16_t>);
static_assert(std::is_signed_v<std::float16_t>);
static_assert(std::is_fundamental_v<std::float16_t>);
static_assert(std::is_scalar_v<std::float16_t>);
static_assert(std::floating_point<std::float16_t>);
static_assert(std::numeric_limits<std::float16_t>::digits == 11);
static_assert(std::numeric_limits<std::float16_t>::max_exponent == 16);
static_assert(std::numeric_limits<std::float16_t>::epsilon() == 0.0009765625f16);
static_assert(std::numeric_limits<std::float16_t>::is_iec559);
static_assert(std::numeric_limits<const std::float16_t>::is_specialized);
#  endif
#  if defined(__STDCPP_FLOAT32_T__)
static_assert(!std::is_same_v<std::float32_t, float>);
static_assert(!std::is_same_v<std::float32_t, double>);
static_assert(!std::is_same_v<std::float32_t, long double>);
static_assert(sizeof(std::float32_t) == 4);
static_assert(std::is_floating_point_v<std::float32_t>);
static_assert(std::is_arithmetic_v<std::float32_t>);
static_assert(std::is_signed_v<std::float32_t>);
static_assert(std::is_fundamental_v<std::float32_t>);
static_assert(std::is_scalar_v<std::float32_t>);
static_assert(std::floating_point<std::float32_t>);
static_assert(std::numeric_limits<std::float32_t>::digits == 24);
static_assert(std::numeric_limits<std::float32_t>::max_exponent == 128);
static_assert(std::numeric_limits<std::float32_t>::epsilon() == 1.1920928955078125e-7f32);
static_assert(std::numeric_limits<std::float32_t>::is_iec559);
static_assert(std::numeric_limits<std::float32_t>::is_specialized);
static_assert(std::is_same_v<decltype(std::float32_t(1) + 1.0f), std::float32_t>);
#  endif
#  if defined(__STDCPP_FLOAT64_T__)
static_assert(!std::is_same_v<std::float64_t, double>);
static_assert(!std::is_same_v<std::float64_t, float>);
static_assert(!std::is_same_v<std::float64_t, long double>);
static_assert(sizeof(std::float64_t) == 8);
static_assert(std::is_floating_point_v<std::float64_t>);
static_assert(std::is_arithmetic_v<std::float64_t>);
static_assert(std::is_signed_v<std::float64_t>);
static_assert(std::is_fundamental_v<std::float64_t>);
static_assert(std::is_scalar_v<std::float64_t>);
static_assert(std::floating_point<std::float64_t>);
static_assert(std::numeric_limits<std::float64_t>::digits == 53);
static_assert(std::numeric_limits<std::float64_t>::max_exponent == 1024);
static_assert(std::numeric_limits<std::float64_t>::epsilon() == 2.220446049250313080847263336181640625e-16f64);
static_assert(std::numeric_limits<std::float64_t>::is_iec559);
static_assert(std::numeric_limits<std::float64_t>::is_specialized);
static_assert(std::is_same_v<decltype(std::float64_t(1) + 1.0), std::float64_t>);
#  endif
#  if defined(__STDCPP_FLOAT128_T__)
static_assert(!std::is_same_v<std::float128_t, long double>);
static_assert(!std::is_same_v<std::float128_t, float>);
static_assert(!std::is_same_v<std::float128_t, double>);
static_assert(sizeof(std::float128_t) == 16);
static_assert(std::is_floating_point_v<std::float128_t>);
static_assert(std::is_arithmetic_v<std::float128_t>);
static_assert(std::is_signed_v<std::float128_t>);
static_assert(std::is_fundamental_v<std::float128_t>);
static_assert(std::is_scalar_v<std::float128_t>);
static_assert(std::numeric_limits<std::float128_t>::digits == 113);
static_assert(std::numeric_limits<std::float128_t>::max_exponent == 16384);
static_assert(std::numeric_limits<std::float128_t>::is_iec559);
static_assert(std::numeric_limits<std::float128_t>::is_specialized);
#  if _LIBCPP_STD_VER >= 26
static_assert(std::hash<std::float128_t>{}(1.0f128) == std::hash<std::float128_t>{}(0.5f128 + 0.5f128));
#  endif
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
static_assert(!std::is_same_v<std::bfloat16_t, float>);
static_assert(!std::is_same_v<std::bfloat16_t, double>);
static_assert(!std::is_same_v<std::bfloat16_t, long double>);
static_assert(sizeof(std::bfloat16_t) == 2);
static_assert(std::is_floating_point_v<std::bfloat16_t>);
static_assert(std::is_arithmetic_v<std::bfloat16_t>);
static_assert(std::is_signed_v<std::bfloat16_t>);
static_assert(std::is_fundamental_v<std::bfloat16_t>);
static_assert(std::is_scalar_v<std::bfloat16_t>);
static_assert(std::numeric_limits<std::bfloat16_t>::digits == 8);
static_assert(std::numeric_limits<std::bfloat16_t>::max_exponent == 128);
static_assert(!std::numeric_limits<std::bfloat16_t>::is_iec559);
static_assert(std::numeric_limits<std::bfloat16_t>::is_specialized);
#  endif
#endif

int main(int, char**) {
#if _LIBCPP_STD_VER >= 23
#  if defined(__STDCPP_FLOAT16_T__)
  assert(std::hash<std::float16_t>{}(1.0f16) == std::hash<std::float16_t>{}(1.0f16));
  assert(std::hash<std::float16_t>{}(1.0f16) == std::hash<std::float16_t>{}(0.5f16 + 0.5f16));
  assert(std::hash<std::float16_t>{}(-0.0f16) == std::hash<std::float16_t>{}(0.0f16));
#  endif
#  if defined(__STDCPP_FLOAT32_T__)
  assert(std::hash<std::float32_t>{}(1.0f32) == std::hash<std::float32_t>{}(1.0f32));
  assert(std::hash<std::float32_t>{}(1.0f32) == std::hash<std::float32_t>{}(0.5f32 + 0.5f32));
#  endif
#  if defined(__STDCPP_FLOAT64_T__)
  assert(std::hash<std::float64_t>{}(1.0f64) == std::hash<std::float64_t>{}(1.0f64));
  assert(std::hash<std::float64_t>{}(1.0f64) == std::hash<std::float64_t>{}(0.5f64 + 0.5f64));
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
  assert(std::hash<std::bfloat16_t>{}(1.0bf16) == std::hash<std::bfloat16_t>{}(1.0bf16));
  assert(std::hash<std::bfloat16_t>{}(1.0bf16) == std::hash<std::bfloat16_t>{}(0.5bf16 + 0.5bf16));
#  endif
#endif
  return 0;
}
