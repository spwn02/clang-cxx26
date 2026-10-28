//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// nextafter, nexttoward and (C++26) nextup and nextdown for the extended floating-point types
// ([cmath.syn], [stdfloat.syn]). The 16-bit formats (float16_t, bfloat16_t) are checked exhaustively
// against an oracle that only knows the ordering of the values: a step in the float format is much
// finer than a step in these formats, so an implementation that goes through float returns its
// argument unchanged.

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdfloat>
#include <utility>
#include <vector>

#include "test_macros.h"

template <int Mant, int ExpBits>
struct Format {
  static constexpr int bias    = (1 << (ExpBits - 1)) - 1;
  static constexpr unsigned pinf = ((1u << ExpBits) - 1) << Mant;
  static bool is_nan(unsigned u) { return (u & 0x7fff) > pinf; }
  static double value(unsigned u) {
    const int e = (u >> Mant) & ((1 << ExpBits) - 1);
    const int m = u & ((1 << Mant) - 1);
    double v;
    if (e == (1 << ExpBits) - 1)
      v = std::numeric_limits<double>::infinity();
    else if (e == 0)
      v = std::ldexp(double(m), 1 - bias - Mant);
    else
      v = std::ldexp(double(m | (1 << Mant)), e - bias - Mant);
    return (u & 0x8000) ? -v : v;
  }
};

template <class T, int Mant, int ExpBits>
void exhaustive() {
  using F = Format<Mant, ExpBits>;
  // every value once, ascending; +0 stands for both zeros
  std::vector<std::pair<double, std::uint16_t>> table;
  for (unsigned u = 0; u < 65536; ++u)
    if (!F::is_nan(u) && u != 0x8000)
      table.push_back({F::value(u), static_cast<std::uint16_t>(u)});
  std::sort(table.begin(), table.end());

  auto oracle = [&](unsigned x, unsigned y) -> std::uint16_t {
    const double vx = F::value(x), vy = F::value(y);
    if (vx == vy)
      return static_cast<std::uint16_t>(y);
    auto it = std::lower_bound(table.begin(), table.end(), std::make_pair(vx, std::uint16_t(0)));
    auto next = vy > vx ? *(it + 1) : *(it - 1);
    if (next.first == 0) // stepping onto zero keeps the sign of x
      return (x & 0x8000) ? 0x8000 : 0;
    return next.second;
  };
  auto bits = [](T v) { return std::bit_cast<std::uint16_t>(v); };
  auto from = [](unsigned u) { return std::bit_cast<T>(static_cast<std::uint16_t>(u)); };

  for (unsigned x = 0; x < 65536; ++x) {
    if (F::is_nan(x))
      continue;
    const unsigned ys[] = {0x0000u, 0x8000u, F::pinf, F::pinf | 0x8000u, x ^ 0x8000u, x, (x + 1) & 0xffff, (x - 1) & 0xffff,
                           bits(T(1)), bits(T(-1)), bits(T(100))};
    for (unsigned y : ys) {
      if (F::is_nan(y))
        continue;
      const std::uint16_t want = oracle(x, y);
      assert(bits(std::nextafter(from(x), from(y))) == want);
      assert(bits(std::nexttoward(from(x), static_cast<long double>(F::value(y)))) == want);
    }
#if TEST_STD_VER >= 26
    const std::uint16_t up = x == F::pinf ? F::pinf : oracle(x, F::pinf);
    const std::uint16_t dn = x == (F::pinf | 0x8000u) ? x : oracle(x, F::pinf | 0x8000u);
    assert(bits(std::nextup(from(x))) == up);
    assert(bits(std::nextdown(from(x))) == dn);
#endif
  }
  // NaNs propagate
  assert(std::isnan(std::nextafter(std::numeric_limits<T>::quiet_NaN(), T(1))));
  assert(std::isnan(std::nextafter(T(1), std::numeric_limits<T>::quiet_NaN())));
  assert(std::isnan(std::nexttoward(T(1), std::numeric_limits<long double>::quiet_NaN())));
#if TEST_STD_VER >= 26
  assert(std::isnan(std::nextup(std::numeric_limits<T>::quiet_NaN())));
  assert(std::isnan(std::nextdown(std::numeric_limits<T>::quiet_NaN())));
#endif
}

#if TEST_STD_VER >= 26
// The edge cases of nextup and nextdown, for every extended type that exists.
template <class T>
constexpr bool edges() {
  using L = std::numeric_limits<T>;
  static_assert(std::is_same_v<decltype(std::nextup(T(1))), T>);
  static_assert(std::is_same_v<decltype(std::nextdown(T(1))), T>);
  if (std::nextup(T(1)) != T(1) + L::epsilon())
    return false;
  if (std::nextdown(T(1)) != T(1) - L::epsilon() / 2)
    return false;
  if (std::nextup(T(0)) != L::denorm_min() || std::nextup(-T(0)) != L::denorm_min())
    return false;
  if (std::nextdown(T(0)) != -L::denorm_min() || std::nextdown(-T(0)) != -L::denorm_min())
    return false;
  if (std::nextup(L::max()) != L::infinity() || std::nextdown(-L::max()) != -L::infinity())
    return false;
  if (std::nextup(L::infinity()) != L::infinity() || std::nextdown(-L::infinity()) != -L::infinity())
    return false;
  if (std::nextup(-L::infinity()) != -L::max() || std::nextdown(L::infinity()) != L::max())
    return false;
  if (!std::signbit(std::nextup(-L::denorm_min())) || std::nextup(-L::denorm_min()) != T(0)) // -0
    return false;
  return true;
}
#endif

int main(int, char**) {
#ifdef __STDCPP_FLOAT16_T__
  exhaustive<std::float16_t, 10, 5>();
  static_assert(std::nextafter(std::float16_t(1), std::float16_t(2)) == std::float16_t(1) + std::numeric_limits<std::float16_t>::epsilon());
#endif
#ifdef __STDCPP_BFLOAT16_T__
  exhaustive<std::bfloat16_t, 7, 8>();
  static_assert(std::nextafter(std::bfloat16_t(1), std::bfloat16_t(2)) == std::bfloat16_t(1) + std::numeric_limits<std::bfloat16_t>::epsilon());
#endif
#if TEST_STD_VER >= 26
#  ifdef __STDCPP_FLOAT16_T__
  static_assert(edges<std::float16_t>());
#  endif
#  ifdef __STDCPP_BFLOAT16_T__
  static_assert(edges<std::bfloat16_t>());
#  endif
#  ifdef __STDCPP_FLOAT32_T__
  static_assert(edges<std::float32_t>());
#  endif
#  ifdef __STDCPP_FLOAT64_T__
  static_assert(edges<std::float64_t>());
#  endif
#  ifdef __STDCPP_FLOAT128_T__
  static_assert(edges<std::float128_t>());
#  endif
#endif
  return 0;
}
