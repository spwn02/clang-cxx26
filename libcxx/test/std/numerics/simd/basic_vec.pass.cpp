//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

#include <bit>
#include <cassert>
#include <array>
#include <concepts>
#include <simd>

int main(int, char**) {
  using vec = std::simd::vec<int, 4>;

  static_assert(vec::size() == 4);
  static_assert(std::same_as<std::simd::rebind_t<float, vec>, std::simd::vec<float, 4>>);
  // [simd.traits]/2: alignment_v is bit_ceil(sizeof(U) * V::size()), not alignof(U) -- a 4-lane
  // int vec's natural alignment is the whole 16-byte object, not a single element's.
  static_assert(std::simd::alignment_v<vec> == std::bit_ceil(sizeof(int) * vec::size()));

  vec a(2);
  vec b([](auto i) { return static_cast<int>(i) + 1; });
  vec c = a + b;
  assert(c[0] == 3 && c[3] == 6);
  assert(std::simd::reduce(c) == 18);
  assert(std::simd::reduce_min(c) == 3);
  assert(std::simd::reduce_max(c) == 6);

  auto mask = c > vec(4);
  assert(std::simd::reduce_count(mask) == 2);
  assert(std::simd::any_of(mask));
  assert(!std::simd::all_of(mask));
  assert(!std::simd::none_of(mask));
  assert(std::simd::reduce_min_index(mask) == 2);
  assert(std::simd::reduce_max_index(mask) == 3);

  auto selected = std::simd::select(mask, c, vec(0));
  assert(selected[0] == 0 && selected[3] == 6);

  std::array<int, 4> input{7, 8, 9, 10};
  auto loaded = std::simd::unchecked_load<vec>(input);
  assert(loaded[0] == 7 && loaded[3] == 10);
  std::array<int, 2> partial_input{11, 12};
  auto partial = std::simd::partial_load<vec>(partial_input);
  assert(partial[0] == 11 && partial[1] == 12 && partial[2] == 0);
  std::array<int, 4> output{};
  std::simd::unchecked_store(loaded, output, mask);
  assert(output[0] == 0 && output[2] == 9 && output[3] == 10);

  auto [low, high] = std::simd::minmax(c, vec(4));
  assert(low[0] == 3 && high[0] == 4);
  assert(std::simd::clamp(c, vec(4), vec(5))[0] == 4);
  auto pieces = std::simd::chunk<2>(c);
  static_assert(std::tuple_size_v<decltype(pieces)> == 2);
  assert(pieces[0][0] == 3 && pieces[1][1] == 6);
  auto joined = std::simd::cat(pieces[0], pieces[1]);
  static_assert(decltype(joined)::size() == 4);
  assert(joined[0] == 3 && joined[3] == 6);

  auto reversed = std::simd::permute<4>(joined, [](auto i) { return 3 - static_cast<int>(i); });
  assert(reversed[0] == 6 && reversed[3] == 3);
  auto with_zero = std::simd::permute<4>(joined, [](auto i) {
    return i == 1 ? std::simd::zero_element : static_cast<int>(i);
  });
  assert(with_zero[0] == 3 && with_zero[1] == 0);

  // simd::where is a TS-era extension, absent from N5050. select() is the closest N5050-conforming
  // replacement for masked writes, but it is not a drop-in equivalent: select(k, x, y) is a value
  // select, not a masked-store, so x is fully computed on every lane (including the ones the mask
  // discards) rather than left unevaluated there the way `where` would. That distinction is
  // invisible in this test's own arithmetic (c - vec(2) is defined and side-effect-free everywhere),
  // but would matter for an expression with masked-off lanes that are individually invalid.
  c = std::simd::select(mask, vec(42), c);
  assert(c[0] == 3 && c[1] == 4 && c[2] == 42 && c[3] == 42);
  c = std::simd::select(mask, c - vec(2), c);
  assert(c[2] == 40 && c[3] == 40);

  std::simd::vec<float, 4> floats(1.5f);
  assert((floats + floats)[2] == 3.0f);
  assert(std::simd::sqrt(std::simd::vec<float, 4>(4.0f))[1] == 2.0f);
  assert(std::simd::fma(floats, floats, floats)[0] == 3.75f);

  std::simd::vec<unsigned, 4> bits([](auto i) { return 1u << static_cast<unsigned>(i); });
  assert(std::simd::all_of(std::simd::has_single_bit(bits)));
  assert(std::simd::bit_width(bits)[3] == 4);
  assert(std::simd::popcount(std::simd::rotl(bits, 1))[0] == 1);
}
