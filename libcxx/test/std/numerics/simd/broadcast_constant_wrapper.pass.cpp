//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// P4012R1: the broadcast constructor accepts a constexpr-wrapper-like argument whose value is representable by value_type,
// whatever the type of that value is; an arithmetic argument has to be of a value-preserving type.

#include <cassert>
#include <concepts>
#include <simd>
#include <type_traits>
#include <utility>

using fvec = std::simd::vec<float, 4>;
using ivec = std::simd::vec<int, 4>;
using bvec = std::simd::vec<unsigned char, 4>;

template <class V, class T>
concept can_broadcast = std::is_constructible_v<V, T>;

// a double constant is representable by float; a plain double is not a value-preserving argument
static_assert(can_broadcast<fvec, decltype(std::cw<1.5>)>);
static_assert(can_broadcast<fvec, decltype(std::cw<0.5>)>);
static_assert(!can_broadcast<fvec, double>);
static_assert(can_broadcast<fvec, float>);
static_assert(can_broadcast<fvec, std::integral_constant<int, 16777216>>);  // 2^24 is representable by float
static_assert(!can_broadcast<fvec, std::integral_constant<int, 16777217>>); // 2^24 + 1 is not
static_assert(can_broadcast<ivec, std::integral_constant<long, 7>>);
static_assert(!can_broadcast<ivec, std::integral_constant<long, 4294967296L>>);
static_assert(can_broadcast<bvec, std::integral_constant<int, 255>>);
static_assert(!can_broadcast<bvec, std::integral_constant<int, 256>>);
static_assert(!can_broadcast<bvec, std::integral_constant<int, -1>>); // changes sign
static_assert(!can_broadcast<ivec, std::integral_constant<double, 2.5>>);
static_assert(can_broadcast<ivec, std::integral_constant<double, 3.0>>);
static_assert(can_broadcast<fvec, std::integral_constant<double, 0.1>> == false); // 0.1 is not exactly a float

int main(int, char**) {
  fvec f(std::cw<1.5>);
  for (int i = 0; i < 4; ++i)
    assert(f[i] == 1.5f);
  bvec b(std::integral_constant<int, 200>{});
  for (int i = 0; i < 4; ++i)
    assert(b[i] == 200);
  return 0;
}
