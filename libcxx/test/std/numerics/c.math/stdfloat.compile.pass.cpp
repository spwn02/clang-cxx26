//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cmath>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

static_assert(std::is_same_v<decltype(std::pow(std::float32_t(2), 2.0f)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::sqrt(std::float32_t(4))), std::float32_t>);
static_assert(std::is_same_v<decltype(std::sin(std::float64_t(1))), std::float64_t>);
static_assert(std::is_same_v<decltype(std::fabs(std::float16_t(1))), std::float16_t>);
static_assert(std::is_same_v<decltype(std::hypot(std::bfloat16_t(1), std::bfloat16_t(2))), std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::abs(std::float128_t(1))), std::float128_t>);

#define CHECK_UNARY(name, type) static_assert(std::is_same_v<decltype(std::name(type(1))), type>)
#define CHECK_BINARY(name, type) static_assert(std::is_same_v<decltype(std::name(type(1), type(2))), type>)
CHECK_UNARY(acos, std::float16_t);
CHECK_UNARY(asin, std::float16_t);
CHECK_UNARY(atan, std::float16_t);
CHECK_UNARY(acosh, std::float16_t);
CHECK_UNARY(asinh, std::float16_t);
CHECK_UNARY(atanh, std::float16_t);
CHECK_UNARY(cos, std::float16_t);
CHECK_UNARY(cosh, std::float16_t);
CHECK_UNARY(sin, std::float16_t);
CHECK_UNARY(sinh, std::float16_t);
CHECK_UNARY(tan, std::float16_t);
CHECK_UNARY(tanh, std::float16_t);
CHECK_UNARY(exp, std::float16_t);
CHECK_UNARY(exp2, std::float16_t);
CHECK_UNARY(expm1, std::float16_t);
CHECK_UNARY(log, std::float16_t);
CHECK_UNARY(log10, std::float16_t);
CHECK_UNARY(log1p, std::float16_t);
CHECK_UNARY(log2, std::float16_t);
CHECK_UNARY(logb, std::float16_t);
CHECK_UNARY(cbrt, std::float16_t);
CHECK_UNARY(ceil, std::float16_t);
CHECK_UNARY(floor, std::float16_t);
CHECK_UNARY(nearbyint, std::float16_t);
CHECK_UNARY(rint, std::float16_t);
CHECK_UNARY(round, std::float16_t);
CHECK_UNARY(trunc, std::float16_t);
CHECK_UNARY(erf, std::float16_t);
CHECK_UNARY(erfc, std::float16_t);
CHECK_UNARY(tgamma, std::float16_t);
CHECK_UNARY(lgamma, std::float16_t);
CHECK_UNARY(sqrt, std::float16_t);
CHECK_UNARY(abs, std::float16_t);
CHECK_UNARY(fabs, std::float16_t);
CHECK_BINARY(atan2, std::float16_t);
CHECK_BINARY(copysign, std::float16_t);
CHECK_BINARY(fdim, std::float16_t);
CHECK_BINARY(fmax, std::float16_t);
CHECK_BINARY(fmin, std::float16_t);
CHECK_BINARY(fmod, std::float16_t);
CHECK_BINARY(hypot, std::float16_t);
CHECK_BINARY(nextafter, std::float16_t);
CHECK_BINARY(pow, std::float16_t);
CHECK_BINARY(remainder, std::float16_t);
static_assert(
    std::is_same_v<decltype(std::fma(std::float16_t(1), std::float16_t(2), std::float16_t(3))), std::float16_t>);
static_assert(std::is_same_v<decltype(std::ldexp(std::float16_t(1), 1)), std::float16_t>);
static_assert(std::is_same_v<decltype(std::frexp(std::float16_t(1), (int*)nullptr)), std::float16_t>);
static_assert(std::is_same_v<decltype(std::modf(std::float16_t(1), (std::float16_t*)nullptr)), std::float16_t>);
static_assert(std::is_same_v<decltype(std::scalbn(std::float16_t(1), 1)), std::float16_t>);
static_assert(std::is_same_v<decltype(std::nexttoward(std::float16_t(1), 2.0L)), std::float16_t>);
static_assert(
    std::is_same_v<decltype(std::remquo(std::float16_t(1), std::float16_t(2), (int*)nullptr)), std::float16_t>);
static_assert(
    std::is_same_v<decltype(std::lerp(std::float16_t(1), std::float16_t(2), std::float16_t(3))), std::float16_t>);
CHECK_UNARY(acos, std::bfloat16_t);
CHECK_UNARY(asin, std::bfloat16_t);
CHECK_UNARY(atan, std::bfloat16_t);
CHECK_UNARY(acosh, std::bfloat16_t);
CHECK_UNARY(asinh, std::bfloat16_t);
CHECK_UNARY(atanh, std::bfloat16_t);
CHECK_UNARY(cos, std::bfloat16_t);
CHECK_UNARY(cosh, std::bfloat16_t);
CHECK_UNARY(sin, std::bfloat16_t);
CHECK_UNARY(sinh, std::bfloat16_t);
CHECK_UNARY(tan, std::bfloat16_t);
CHECK_UNARY(tanh, std::bfloat16_t);
CHECK_UNARY(exp, std::bfloat16_t);
CHECK_UNARY(exp2, std::bfloat16_t);
CHECK_UNARY(expm1, std::bfloat16_t);
CHECK_UNARY(log, std::bfloat16_t);
CHECK_UNARY(log10, std::bfloat16_t);
CHECK_UNARY(log1p, std::bfloat16_t);
CHECK_UNARY(log2, std::bfloat16_t);
CHECK_UNARY(logb, std::bfloat16_t);
CHECK_UNARY(cbrt, std::bfloat16_t);
CHECK_UNARY(ceil, std::bfloat16_t);
CHECK_UNARY(floor, std::bfloat16_t);
CHECK_UNARY(nearbyint, std::bfloat16_t);
CHECK_UNARY(rint, std::bfloat16_t);
CHECK_UNARY(round, std::bfloat16_t);
CHECK_UNARY(trunc, std::bfloat16_t);
CHECK_UNARY(erf, std::bfloat16_t);
CHECK_UNARY(erfc, std::bfloat16_t);
CHECK_UNARY(tgamma, std::bfloat16_t);
CHECK_UNARY(lgamma, std::bfloat16_t);
CHECK_UNARY(sqrt, std::bfloat16_t);
CHECK_UNARY(abs, std::bfloat16_t);
CHECK_UNARY(fabs, std::bfloat16_t);
CHECK_BINARY(atan2, std::bfloat16_t);
CHECK_BINARY(copysign, std::bfloat16_t);
CHECK_BINARY(fdim, std::bfloat16_t);
CHECK_BINARY(fmax, std::bfloat16_t);
CHECK_BINARY(fmin, std::bfloat16_t);
CHECK_BINARY(fmod, std::bfloat16_t);
CHECK_BINARY(hypot, std::bfloat16_t);
CHECK_BINARY(nextafter, std::bfloat16_t);
CHECK_BINARY(pow, std::bfloat16_t);
CHECK_BINARY(remainder, std::bfloat16_t);
static_assert(
    std::is_same_v<decltype(std::fma(std::bfloat16_t(1), std::bfloat16_t(2), std::bfloat16_t(3))), std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::ldexp(std::bfloat16_t(1), 1)), std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::frexp(std::bfloat16_t(1), (int*)nullptr)), std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::modf(std::bfloat16_t(1), (std::bfloat16_t*)nullptr)), std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::scalbn(std::bfloat16_t(1), 1)), std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::nexttoward(std::bfloat16_t(1), 2.0L)), std::bfloat16_t>);
static_assert(
    std::is_same_v<decltype(std::remquo(std::bfloat16_t(1), std::bfloat16_t(2), (int*)nullptr)), std::bfloat16_t>);
static_assert(
    std::is_same_v<decltype(std::lerp(std::bfloat16_t(1), std::bfloat16_t(2), std::bfloat16_t(3))), std::bfloat16_t>);
CHECK_UNARY(acos, std::float32_t);
CHECK_UNARY(asin, std::float32_t);
CHECK_UNARY(atan, std::float32_t);
CHECK_UNARY(acosh, std::float32_t);
CHECK_UNARY(asinh, std::float32_t);
CHECK_UNARY(atanh, std::float32_t);
CHECK_UNARY(cos, std::float32_t);
CHECK_UNARY(cosh, std::float32_t);
CHECK_UNARY(sin, std::float32_t);
CHECK_UNARY(sinh, std::float32_t);
CHECK_UNARY(tan, std::float32_t);
CHECK_UNARY(tanh, std::float32_t);
CHECK_UNARY(exp, std::float32_t);
CHECK_UNARY(exp2, std::float32_t);
CHECK_UNARY(expm1, std::float32_t);
CHECK_UNARY(log, std::float32_t);
CHECK_UNARY(log10, std::float32_t);
CHECK_UNARY(log1p, std::float32_t);
CHECK_UNARY(log2, std::float32_t);
CHECK_UNARY(logb, std::float32_t);
CHECK_UNARY(cbrt, std::float32_t);
CHECK_UNARY(ceil, std::float32_t);
CHECK_UNARY(floor, std::float32_t);
CHECK_UNARY(nearbyint, std::float32_t);
CHECK_UNARY(rint, std::float32_t);
CHECK_UNARY(round, std::float32_t);
CHECK_UNARY(trunc, std::float32_t);
CHECK_UNARY(erf, std::float32_t);
CHECK_UNARY(erfc, std::float32_t);
CHECK_UNARY(tgamma, std::float32_t);
CHECK_UNARY(lgamma, std::float32_t);
CHECK_UNARY(sqrt, std::float32_t);
CHECK_UNARY(abs, std::float32_t);
CHECK_UNARY(fabs, std::float32_t);
CHECK_BINARY(atan2, std::float32_t);
CHECK_BINARY(copysign, std::float32_t);
CHECK_BINARY(fdim, std::float32_t);
CHECK_BINARY(fmax, std::float32_t);
CHECK_BINARY(fmin, std::float32_t);
CHECK_BINARY(fmod, std::float32_t);
CHECK_BINARY(hypot, std::float32_t);
CHECK_BINARY(nextafter, std::float32_t);
CHECK_BINARY(pow, std::float32_t);
CHECK_BINARY(remainder, std::float32_t);
static_assert(
    std::is_same_v<decltype(std::fma(std::float32_t(1), std::float32_t(2), std::float32_t(3))), std::float32_t>);
static_assert(std::is_same_v<decltype(std::ldexp(std::float32_t(1), 1)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::frexp(std::float32_t(1), (int*)nullptr)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::modf(std::float32_t(1), (std::float32_t*)nullptr)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::scalbn(std::float32_t(1), 1)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::nexttoward(std::float32_t(1), 2.0L)), std::float32_t>);
static_assert(
    std::is_same_v<decltype(std::remquo(std::float32_t(1), std::float32_t(2), (int*)nullptr)), std::float32_t>);
static_assert(
    std::is_same_v<decltype(std::lerp(std::float32_t(1), std::float32_t(2), std::float32_t(3))), std::float32_t>);
CHECK_UNARY(acos, std::float64_t);
CHECK_UNARY(asin, std::float64_t);
CHECK_UNARY(atan, std::float64_t);
CHECK_UNARY(acosh, std::float64_t);
CHECK_UNARY(asinh, std::float64_t);
CHECK_UNARY(atanh, std::float64_t);
CHECK_UNARY(cos, std::float64_t);
CHECK_UNARY(cosh, std::float64_t);
CHECK_UNARY(sin, std::float64_t);
CHECK_UNARY(sinh, std::float64_t);
CHECK_UNARY(tan, std::float64_t);
CHECK_UNARY(tanh, std::float64_t);
CHECK_UNARY(exp, std::float64_t);
CHECK_UNARY(exp2, std::float64_t);
CHECK_UNARY(expm1, std::float64_t);
CHECK_UNARY(log, std::float64_t);
CHECK_UNARY(log10, std::float64_t);
CHECK_UNARY(log1p, std::float64_t);
CHECK_UNARY(log2, std::float64_t);
CHECK_UNARY(logb, std::float64_t);
CHECK_UNARY(cbrt, std::float64_t);
CHECK_UNARY(ceil, std::float64_t);
CHECK_UNARY(floor, std::float64_t);
CHECK_UNARY(nearbyint, std::float64_t);
CHECK_UNARY(rint, std::float64_t);
CHECK_UNARY(round, std::float64_t);
CHECK_UNARY(trunc, std::float64_t);
CHECK_UNARY(erf, std::float64_t);
CHECK_UNARY(erfc, std::float64_t);
CHECK_UNARY(tgamma, std::float64_t);
CHECK_UNARY(lgamma, std::float64_t);
CHECK_UNARY(sqrt, std::float64_t);
CHECK_UNARY(abs, std::float64_t);
CHECK_UNARY(fabs, std::float64_t);
CHECK_BINARY(atan2, std::float64_t);
CHECK_BINARY(copysign, std::float64_t);
CHECK_BINARY(fdim, std::float64_t);
CHECK_BINARY(fmax, std::float64_t);
CHECK_BINARY(fmin, std::float64_t);
CHECK_BINARY(fmod, std::float64_t);
CHECK_BINARY(hypot, std::float64_t);
CHECK_BINARY(nextafter, std::float64_t);
CHECK_BINARY(pow, std::float64_t);
CHECK_BINARY(remainder, std::float64_t);
static_assert(
    std::is_same_v<decltype(std::fma(std::float64_t(1), std::float64_t(2), std::float64_t(3))), std::float64_t>);
static_assert(std::is_same_v<decltype(std::ldexp(std::float64_t(1), 1)), std::float64_t>);
static_assert(std::is_same_v<decltype(std::frexp(std::float64_t(1), (int*)nullptr)), std::float64_t>);
static_assert(std::is_same_v<decltype(std::modf(std::float64_t(1), (std::float64_t*)nullptr)), std::float64_t>);
static_assert(std::is_same_v<decltype(std::scalbn(std::float64_t(1), 1)), std::float64_t>);
static_assert(std::is_same_v<decltype(std::nexttoward(std::float64_t(1), 2.0L)), std::float64_t>);
static_assert(
    std::is_same_v<decltype(std::remquo(std::float64_t(1), std::float64_t(2), (int*)nullptr)), std::float64_t>);
static_assert(
    std::is_same_v<decltype(std::lerp(std::float64_t(1), std::float64_t(2), std::float64_t(3))), std::float64_t>);

template <class Left, class Right>
concept HasPow = requires(Left left, Right right) { std::pow(left, right); };
static_assert(!HasPow<std::float16_t, std::bfloat16_t>);
static_assert(std::is_same_v<decltype(std::isgreater(std::float32_t(2), 1.0f)), bool>);
static_assert(std::is_same_v<decltype(std::isless(std::float16_t(1), std::float16_t(2))), bool>);
static_assert(std::is_same_v<decltype(std::pow(std::float32_t(2), 2)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::fma(std::float32_t(2), 2.0f, 1.0f)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::fabs(std::float128_t(1))), std::float128_t>);
static_assert(std::is_same_v<decltype(std::sqrt(std::float128_t(1))), std::float128_t>);
static_assert(std::isfinite(std::float128_t(1)));
static_assert(
    std::is_same_v<decltype(std::hypot(std::float16_t(1), std::float16_t(2), std::float16_t(3))), std::float16_t>);
static_assert(std::fabs(std::float32_t(-2)) == std::float32_t(2));
static_assert(std::isgreater(std::float128_t(2), std::float128_t(1)));
static_assert(std::isless(std::float32_t(1), 2.0f));
static_assert(std::is_same_v<decltype(std::ilogb(std::float32_t(2))), int>);
static_assert(std::is_same_v<decltype(std::lrint(std::float32_t(2))), long>);
static_assert(std::is_same_v<decltype(std::lround(std::float32_t(2))), long>);
static_assert(std::is_same_v<decltype(std::llrint(std::float32_t(2))), long long>);
static_assert(std::is_same_v<decltype(std::llround(std::float32_t(2))), long long>);
static_assert(std::is_same_v<decltype(std::scalbln(std::float32_t(2), 1L)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::nextafter(std::float32_t(1), 2.0f)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::atan2(std::float32_t(1), 2.0f)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::hypot(std::float32_t(1), 2.0f)), std::float32_t>);
static_assert(std::is_same_v<decltype(std::lerp(std::float32_t(1), 2.0f, 0.5f)), std::float32_t>);
static_assert(std::fabs(std::float128_t(-2)) == std::float128_t(2));
template <class Left, class Right>
concept HasIsGreater = requires(Left left, Right right) { std::isgreater(left, right); };
static_assert(!HasIsGreater<std::float16_t, std::bfloat16_t>);
#define CHECK_UNORDERED_COMPARE(name)                                                                   \
  static_assert(![]<class Left, class Right>() { return requires(Left left, Right right) { std::name(left, right); }; } \
                    .template operator()<std::float16_t, std::bfloat16_t>())
CHECK_UNORDERED_COMPARE(isgreater);
CHECK_UNORDERED_COMPARE(isgreaterequal);
CHECK_UNORDERED_COMPARE(isless);
CHECK_UNORDERED_COMPARE(islessequal);
CHECK_UNORDERED_COMPARE(islessgreater);
CHECK_UNORDERED_COMPARE(isunordered);
#undef CHECK_UNORDERED_COMPARE
#define CHECK_UNORDERED_BINARY(name)                                                                     \
  static_assert(![]<class Left, class Right>() { return requires(Left left, Right right) { std::name(left, right); }; } \
                    .template operator()<std::float16_t, std::bfloat16_t>())
CHECK_UNORDERED_BINARY(atan2);
CHECK_UNORDERED_BINARY(copysign);
CHECK_UNORDERED_BINARY(fdim);
CHECK_UNORDERED_BINARY(fmax);
CHECK_UNORDERED_BINARY(fmin);
CHECK_UNORDERED_BINARY(fmod);
CHECK_UNORDERED_BINARY(hypot);
CHECK_UNORDERED_BINARY(nextafter);
CHECK_UNORDERED_BINARY(pow);
CHECK_UNORDERED_BINARY(remainder);
#undef CHECK_UNORDERED_BINARY
template <class Left, class Right>
concept HasFma = requires(Left left, Right right) { std::fma(left, right, left); };
static_assert(!HasFma<std::float16_t, std::bfloat16_t>);
