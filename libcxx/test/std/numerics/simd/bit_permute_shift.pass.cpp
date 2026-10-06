//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// P3772R2 (bit_reverse, bit_repeat, bit_compress, bit_expand) and P3793R2 (shl, shr) for simd::vec, each checked
// lane by lane against the scalar function of <bit>.

#include <bit>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <simd>

#ifndef __cpp_lib_simd_bitops
#  error __cpp_lib_simd_bitops should be defined
#endif
#if __cpp_lib_simd_bitops != 202607L
#  error __cpp_lib_simd_bitops should have the value 202607L
#endif
#if __cpp_lib_simd != 202606L
#  error __cpp_lib_simd should have the value 202606L
#endif

using uvec = std::simd::vec<std::uint32_t, 4>;
using ivec = std::simd::vec<std::int32_t, 4>;

int main(int, char**) {
  uvec v([](auto i) { return 0x12345678u + static_cast<unsigned>(i) * 0x01010101u; });
  uvec m([](auto i) { return 0xF0F0F0F0u >> static_cast<unsigned>(i); });

  {
    auto r = std::simd::bit_reverse(v);
    static_assert(std::same_as<decltype(r), uvec>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::bit_reverse(v[i]));
    static_assert(noexcept(std::simd::bit_reverse(v)));
  }
  {
    uvec l([](auto i) { return 4u + static_cast<unsigned>(i); });
    auto r = std::simd::bit_repeat(uvec(0x3u), l);
    static_assert(std::same_as<decltype(r), uvec>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::bit_repeat(0x3u, static_cast<int>(l[i])));
    auto s = std::simd::bit_repeat(uvec(0x5u), 3);
    for (int i = 0; i < 4; ++i)
      assert(s[i] == std::bit_repeat(0x5u, 3));
  }
  {
    auto c = std::simd::bit_compress(v, m);
    auto e = std::simd::bit_expand(v, m);
    static_assert(std::same_as<decltype(c), uvec> && std::same_as<decltype(e), uvec>);
    for (int i = 0; i < 4; ++i) {
      assert(c[i] == std::bit_compress(v[i], m[i]));
      assert(e[i] == std::bit_expand(v[i], m[i]));
    }
    auto cs = std::simd::bit_compress(v, 0x00FF00FFu);
    auto es = std::simd::bit_expand(v, 0x00FF00FFu);
    for (int i = 0; i < 4; ++i) {
      assert(cs[i] == std::bit_compress(v[i], 0x00FF00FFu));
      assert(es[i] == std::bit_expand(v[i], 0x00FF00FFu));
    }
  }

  // shl/shr: per-lane counts of an equally wide integer vector (signed or unsigned), or one scalar count
  {
    ivec counts([](auto i) { return static_cast<int>(i) * 12 - 13; }); // {-13, -1, 11, 23}
    auto l = std::simd::shl(v, counts);
    auto r = std::simd::shr(v, counts);
    static_assert(std::same_as<decltype(l), uvec> && std::same_as<decltype(r), uvec>);
    for (int i = 0; i < 4; ++i) {
      assert(l[i] == std::shl(v[i], counts[i]));
      assert(r[i] == std::shr(v[i], counts[i]));
    }
    auto ls = std::simd::shl(v, 40);
    auto rs = std::simd::shr(v, -3);
    for (int i = 0; i < 4; ++i) {
      assert(ls[i] == std::shl(v[i], 40));
      assert(rs[i] == std::shr(v[i], -3));
    }
    ivec s([](auto i) { return static_cast<int>(i) * -17000; });
    auto ss = std::simd::shr(s, std::int8_t(2));
    for (int i = 0; i < 4; ++i)
      assert(ss[i] == std::shr(s[i], std::int8_t(2)));
    static_assert(noexcept(std::simd::shl(v, counts)));
  }
  return 0;
}
