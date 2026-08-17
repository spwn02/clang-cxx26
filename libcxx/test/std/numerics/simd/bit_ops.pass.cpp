//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.bit]: byteswap, bit_ceil, bit_floor, has_single_bit, rotl, rotr, bit_width, countl_zero,
// countl_one, countr_zero, countr_one, popcount -- all lane-wise wrappers over the scalar <bit>
// functions of the same name. This file checks each against its scalar oracle lane-by-lane, plus
// the two structural facts that distinguish this clause from a plain per-lane forward:
//   - byteswap alone is constrained on integral (not unsigned_integral) -- [simd.bit]/1-2.
//   - bit_width/countl_*/countr_*/popcount return rebind_t<make_signed_t<value_type>, V>, a
//     *different* vec type than their argument, not V itself -- [simd.bit]/15-16.

#include <bit>
#include <cassert>
#include <concepts>
#include <simd>

int main(int, char**) {
  using uvec = std::simd::vec<unsigned, 4>;
  using ivec = std::simd::vec<int, 4>;

  uvec bits([](auto i) { return 1u << (static_cast<unsigned>(i) * 2); }); // {1, 4, 16, 64}

  {
    auto r = std::simd::has_single_bit(bits);
    static_assert(std::same_as<decltype(r), uvec::mask_type>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::has_single_bit(bits[i]));
    assert(std::simd::all_of(r));
  }

  {
    auto r = std::simd::bit_ceil(bits);
    static_assert(std::same_as<decltype(r), uvec>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::bit_ceil(bits[i]));
  }
  {
    uvec nonpow2([](auto i) { return static_cast<unsigned>(i) * 3 + 5; }); // {5, 8, 11, 14}
    auto r = std::simd::bit_floor(nonpow2);
    static_assert(std::same_as<decltype(r), uvec>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::bit_floor(nonpow2[i]));
  }

  // rotl/rotr, vec-count overload -- a per-lane shift amount of a distinct (but same-width, same
  // element-size) integral vec type, per [simd.bit]/11-12's V0/V1 split.
  {
    ivec shifts([](auto i) { return static_cast<int>(i) + 1; }); // {1, 2, 3, 4}
    auto rl = std::simd::rotl(bits, shifts);
    static_assert(std::same_as<decltype(rl), uvec>);
    for (int i = 0; i < 4; ++i)
      assert(rl[i] == std::rotl(bits[i], shifts[i]));

    auto rr = std::simd::rotr(bits, shifts);
    for (int i = 0; i < 4; ++i)
      assert(rr[i] == std::rotr(bits[i], shifts[i]));
  }
  // rotl/rotr, scalar-int-count overload.
  {
    auto rl = std::simd::rotl(bits, 3);
    for (int i = 0; i < 4; ++i)
      assert(rl[i] == std::rotl(bits[i], 3));
    auto rr = std::simd::rotr(bits, 3);
    for (int i = 0; i < 4; ++i)
      assert(rr[i] == std::rotr(bits[i], 3));
  }

  // byteswap: integral, not unsigned_integral -- exercise it on a *signed* int vec, which the other
  // names in this clause all reject.
  {
    ivec x([](auto i) { return 0x01020304 + static_cast<int>(i); });
    auto r = std::simd::byteswap(x);
    static_assert(std::same_as<decltype(r), ivec>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::byteswap(x[i]));
  }

  // bit_width/countl_zero/countl_one/countr_zero/countr_one/popcount: [simd.bit]/15-16's
  // return-type deviation. rebind_t<make_signed_t<unsigned>, uvec> is a *signed*-int vec, not uvec
  // itself -- check that as a named fact, not just that the values happen to match.
  {
    auto r = std::simd::bit_width(bits);
    static_assert(std::same_as<decltype(r), std::simd::rebind_t<int, uvec>>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::bit_width(bits[i]));
  }
  {
    auto r = std::simd::countl_zero(bits);
    static_assert(std::same_as<decltype(r), std::simd::rebind_t<int, uvec>>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::countl_zero(bits[i]));
  }
  {
    auto r = std::simd::countl_one(~bits);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::countl_one((~bits)[i]));
  }
  {
    auto r = std::simd::countr_zero(bits);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::countr_zero(bits[i]));
  }
  {
    uvec ones_low([](auto i) { return (1u << (static_cast<unsigned>(i) + 1)) - 1u; }); // {3,7,15,31}
    auto r = std::simd::countr_one(ones_low);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::countr_one(ones_low[i]));
  }
  {
    auto r = std::simd::popcount(bits);
    static_assert(std::same_as<decltype(r), std::simd::rebind_t<int, uvec>>);
    for (int i = 0; i < 4; ++i)
      assert(r[i] == std::popcount(bits[i]));
  }

  // Plain-std:: re-exports (P3287R3's element-wise-overload rule -- [[simd-n5050-port]] memory):
  // must resolve to the same simd overloads without colliding with <bit>'s scalar ones.
  {
    assert(std::popcount(bits[0]) == 1);      // scalar <bit>, unsigned argument
    auto r = std::popcount(bits);             // plain-std:: simd re-export, uvec argument
    static_assert(std::same_as<decltype(r), std::simd::rebind_t<int, uvec>>);
    assert(r[2] == 1);
  }

  return 0;
}
