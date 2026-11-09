//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <simd>

// [simd.creation] (P3319R6): template<class T> constexpr T iota = see below;

#include <cassert>
#include <concepts>
#include <simd>
#include <version>

#ifndef __cpp_lib_simd
#  error "__cpp_lib_simd should be defined"
#endif
static_assert(__cpp_lib_simd == 202606L);

// A vectorizable arithmetic type: iota<T> is T().
static_assert(std::same_as<decltype(std::simd::iota<int>), const int>);
static_assert(std::simd::iota<int> == 0);
static_assert(std::simd::iota<float> == 0.0f);
static_assert(std::simd::iota<unsigned char> == 0);

// A basic_vec: element i is i.
constexpr bool check_vec() {
  using V = std::simd::vec<int, 4>;
  constexpr V v = std::simd::iota<V>;
  for (int i = 0; i != V::size(); ++i)
    if (v[i] != i)
      return false;
  using F = std::simd::vec<float, 8>;
  constexpr F f = std::simd::iota<F>;
  for (int i = 0; i != F::size(); ++i)
    if (f[i] != static_cast<float>(i))
      return false;
  using D = std::simd::vec<int>;
  constexpr D d = std::simd::iota<D>;
  for (int i = 0; i != D::size(); ++i)
    if (d[i] != i)
      return false;
  return true;
}
static_assert(check_vec());

// The element type can be as small as the width allows.
static_assert(std::simd::iota<std::simd::vec<signed char, 64>>[63] == 63);
static_assert(std::simd::iota<std::simd::vec<unsigned char, 64>>[63] == 63);

// Used as a sequence of offsets.
constexpr auto offsets = 5 + 3 * std::simd::iota<std::simd::vec<int, 4>>;
static_assert(offsets[0] == 5 && offsets[1] == 8 && offsets[3] == 14);

int main(int, char**) {
  auto v = std::simd::iota<std::simd::vec<int, 4>>;
  assert(v[0] == 0 && v[3] == 3);
  return 0;
}
