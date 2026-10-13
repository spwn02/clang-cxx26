//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.math]: the multi-argument functions accept calls that mix a vector with scalars. V is deduced from the
// vector operand, the scalar operand is broadcast through the deduced-vec-t<V> parameter and must therefore convert
// to V implicitly (a value-preserving conversion).

#include <cassert>
#include <cmath>
#include <simd>
#include <type_traits>

namespace simd = std::simd;
using V        = simd::vec<float, 4>;
using D        = simd::vec<double, 2>;

template <class... Args>
concept can_hypot = requires(Args... args) { simd::hypot(args...); };
template <class... Args>
concept can_fma = requires(Args... args) { simd::fma(args...); };

// Operands other than the vector convert to it implicitly, so a scalar or vector of a type that converts
// without narrowing is accepted and a narrowing one is not; an operand list without a vector is not a call of
// this overload set.
using D4 = simd::vec<double, 4>;
struct ConvertsToFloat {
  operator float() const { return 1.0f; }
};
static_assert(can_hypot<D, float>);
static_assert(can_hypot<D, int>);
static_assert(can_hypot<V, short>);
static_assert(can_hypot<V, D4>);
static_assert(can_hypot<D4, V>);
static_assert(can_hypot<V, ConvertsToFloat>);
static_assert(can_fma<V, float, D4>);
static_assert(std::is_same_v<decltype(simd::hypot(V(), D4())), D4>);
static_assert(std::is_same_v<decltype(simd::hypot(D4(), V())), D4>);
static_assert(std::is_same_v<decltype(simd::fma(V(), 1.0f, D4())), D4>);
static_assert(std::is_same_v<decltype(simd::hypot(D(), 1)), D>);
static_assert(can_hypot<V, float>);
static_assert(can_hypot<float, V>);
static_assert(!can_hypot<V, double>);
static_assert(!can_hypot<V, int>);
static_assert(!can_hypot<V, D>);
static_assert(!can_hypot<float, float>);
static_assert(can_fma<V, float, float>);
static_assert(can_fma<float, V, float>);
static_assert(can_fma<float, float, V>);
static_assert(can_fma<V, V, float>);
static_assert(can_fma<V, float, V>);
static_assert(can_fma<float, V, V>);
static_assert(!can_fma<V, double, float>);

// The result type is that of the vector operand.
static_assert(std::is_same_v<decltype(simd::hypot(V(), 1.0f)), V>);
static_assert(std::is_same_v<decltype(simd::isgreater(V(), 1.0f)), V::mask_type>);
static_assert(std::is_same_v<decltype(simd::fma(1.0, D(), 1.0)), D>);

int main(int, char**) {
  V v(1.5f), w(2.0f), k(0.3f), phi(0.4f);

  // Binary, both operand positions.
  assert(simd::hypot(V(3.0f), 4.0f)[0] == 5.0f);
  assert(simd::hypot(3.0f, V(4.0f))[0] == 5.0f);
  assert(simd::pow(v, 2.0f)[3] == std::pow(1.5f, 2.0f));
  assert(simd::atan2(2.0f, w)[1] == std::atan2(2.0f, 2.0f));
  assert(simd::fmax(v, 1.0f)[2] == 1.5f);
  assert(simd::copysign(v, -1.0f)[0] == -1.5f);

  // Ternary: every placement of the scalar operands.
  assert(simd::fma(v, 2.0f, 1.0f)[0] == std::fma(1.5f, 2.0f, 1.0f));
  assert(simd::fma(2.0f, v, w)[0] == std::fma(2.0f, 1.5f, 2.0f));
  assert(simd::fma(2.0f, 1.5f, w)[0] == std::fma(2.0f, 1.5f, 2.0f));
  assert(simd::fma(v, w, 3.0f)[0] == std::fma(1.5f, 2.0f, 3.0f));
  assert(simd::lerp(v, 3.0f, 0.5f)[0] == std::lerp(1.5f, 3.0f, 0.5f));
  assert(simd::hypot(v, 2.0f, 3.0f)[0] == std::hypot(1.5f, 2.0f, 3.0f));

  // Classification functions returning a mask.
  assert(simd::isgreater(v, 1.0f)[0]);
  assert(!simd::isless(v, 1.0f)[0]);
  assert(simd::isunordered(v, std::nanf(""))[0]);

  // Special functions.
  assert(simd::beta(v, 2.0f)[0] == std::beta(1.5f, 2.0f));
  assert(simd::cyl_bessel_j(1.5f, w)[0] == std::cyl_bessel_j(1.5f, 2.0f));
  assert(simd::ellint_1(k, 0.5f)[0] == std::ellint_1(0.3f, 0.5f));
  assert(simd::ellint_3(k, 0.5f, phi)[0] == std::ellint_3(0.3f, 0.5f, 0.4f));
  assert(simd::comp_ellint_3(k, 0.5f)[0] == std::comp_ellint_3(0.3f, 0.5f));

  // Vector-only calls are unchanged (and not ambiguous).
  assert(simd::hypot(v, w)[0] == std::hypot(1.5f, 2.0f));
  assert(simd::fma(v, w, v)[0] == std::fma(1.5f, 2.0f, 1.5f));
  assert(simd::beta(v, w)[0] == std::beta(1.5f, 2.0f));

  return 0;
}
