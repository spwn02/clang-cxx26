//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.general]/2: the vectorizable types include std::float16_t, std::float32_t and std::float64_t if defined, and
// complex<T> of those. std::bfloat16_t and std::float128_t are not on the list. The converting constructor of basic_vec
// is explicit when the conversion is not value-preserving or the floating-point conversion rank of the source is
// greater ([simd.ctor]), so float32_t and float on one side and float64_t and double on the other convert implicitly
// in both directions, because an extended type has the conversion rank of the standard type of the same format.

#include <cassert>
#include <complex>
#include <cstdint>
#include <functional>
#include <simd>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

namespace s = std::simd;

template <class T>
void run() {
  using V = s::vec<T, 4>;
  static_assert(std::is_default_constructible_v<V>);
  static_assert(V::size() == 4);
  static_assert(std::is_same_v<typename V::value_type, T>);
  V a([](auto i) { return static_cast<T>(i + 1); }); // 1 2 3 4
  V b(static_cast<T>(2));
  V c = a * b + a; // 3 6 9 12
  assert(c[0] == static_cast<T>(3) && c[3] == static_cast<T>(12));
  assert(s::reduce(a) == static_cast<T>(10));
  assert(s::reduce(a, std::multiplies<>{}) == static_cast<T>(24));
  assert(s::reduce_min(a) == static_cast<T>(1) && s::reduce_max(a) == static_cast<T>(4));
  auto m = a > V(static_cast<T>(2));
  assert(s::reduce_count(m) == 2 && s::any_of(m) && !s::all_of(m));
  V d = s::select(m, a, V(static_cast<T>(0)));
  assert(d[0] == static_cast<T>(0) && d[3] == static_cast<T>(4));
  assert((a / b)[1] == static_cast<T>(1));
  V neg = -a;
  assert(neg[2] == static_cast<T>(-3) && s::abs(neg)[3] == static_cast<T>(4));
  assert(s::sqrt(V([](auto i) { return static_cast<T>((i + 1) * (i + 1)); }))[2] == static_cast<T>(3));
  assert(s::min(a, b)[3] == static_cast<T>(2) && s::max(a, b)[0] == static_cast<T>(2));
  T in[4] = {static_cast<T>(5), static_cast<T>(6), static_cast<T>(7), static_cast<T>(8)};
  V l = s::unchecked_load<V>(in);
  assert(l[0] == static_cast<T>(5) && l[3] == static_cast<T>(8));
  T out[4] = {};
  s::unchecked_store(l + a, out);
  assert(out[0] == static_cast<T>(6) && out[3] == static_cast<T>(12));
  assert(s::permute(a, [](auto i) { return 3 - i; })[0] == static_cast<T>(4));
}

int main(int, char**) {
#ifdef __STDCPP_FLOAT16_T__
  run<std::float16_t>();
  static_assert(std::is_default_constructible_v<s::vec<std::complex<std::float16_t>, 2>>);
#endif
#ifdef __STDCPP_FLOAT32_T__
  run<std::float32_t>();
  static_assert(std::is_default_constructible_v<s::vec<std::complex<std::float32_t>, 2>>);
#endif
#ifdef __STDCPP_FLOAT64_T__
  run<std::float64_t>();
  static_assert(std::is_default_constructible_v<s::vec<std::complex<std::float64_t>, 2>>);
#endif

  // not vectorizable
#ifdef __STDCPP_BFLOAT16_T__
  static_assert(!std::is_default_constructible_v<s::vec<std::bfloat16_t, 4>>);
  static_assert(!std::is_default_constructible_v<s::vec<std::complex<std::bfloat16_t>, 2>>);
#endif
#ifdef __STDCPP_FLOAT128_T__
  static_assert(!std::is_default_constructible_v<s::vec<std::float128_t, 4>>);
#endif
  static_assert(!std::is_default_constructible_v<s::vec<long double, 4>>);

#if defined(__STDCPP_FLOAT32_T__) && defined(__STDCPP_FLOAT64_T__) && defined(__STDCPP_FLOAT16_T__)
  using F32 = s::vec<std::float32_t, 4>;
  using F64 = s::vec<std::float64_t, 4>;
  using H   = s::vec<std::float16_t, 4>;
  using F   = s::vec<float, 4>;
  using D   = s::vec<double, 4>;
  // the same format: implicit both ways
  static_assert(std::is_convertible_v<F32, F> && std::is_convertible_v<F, F32>);
  static_assert(std::is_convertible_v<F64, D> && std::is_convertible_v<D, F64>);
  // widening: implicit
  static_assert(std::is_convertible_v<H, F32> && std::is_convertible_v<H, F64> && std::is_convertible_v<F32, F64>);
  // narrowing: explicit only
  static_assert(!std::is_convertible_v<F64, F32> && std::is_constructible_v<F32, F64>);
  static_assert(!std::is_convertible_v<F32, H> && std::is_constructible_v<H, F32>);
  // an integer converts implicitly only when it fits in the significand
  static_assert(std::is_convertible_v<s::vec<std::int16_t, 4>, F32>);
  static_assert(!std::is_convertible_v<s::vec<std::int32_t, 4>, F32> && std::is_constructible_v<F32, s::vec<std::int32_t, 4>>);
  static_assert(std::is_convertible_v<s::vec<std::int32_t, 4>, F64>);
  F32 x(std::float32_t(1.5));
  assert(static_cast<F>(x)[0] == 1.5f);
#endif
  return 0;
}
