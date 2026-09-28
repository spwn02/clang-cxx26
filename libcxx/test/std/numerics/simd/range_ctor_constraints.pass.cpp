//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.ctor]/12: range constructors require a constant ranges::size(r) equal to
// the SIMD width. The same constant-size requirement applies to range CTAD.

#include <array>
#include <cassert>
#include <concepts>
#include <ranges>
#include <simd>
#include <span>
#include <vector>

template <class V, class R>
concept constructible_from_range = requires(R&& r) { V(static_cast<R&&>(r)); };

template <class V, class R>
concept masked_constructible_from_range = requires(R&& r, typename V::mask_type mask) {
  V(static_cast<R&&>(r), mask);
};

template <class R>
concept deducible_from_range = requires(R&& r) { std::simd::basic_vec(static_cast<R&&>(r)); };

template <class T, std::size_t N>
struct fixed_range {
  T* first;

  fixed_range() = delete;
  constexpr explicit fixed_range(T* p) : first(p) {}
  constexpr T* begin() const { return first; }
  constexpr T* end() const { return first + N; }
  constexpr std::size_t size() const { return N; }
};

template <class T>
struct runtime_range {
  T* first;
  std::size_t count;

  constexpr T* begin() const { return first; }
  constexpr T* end() const { return first + count; }
  constexpr std::size_t size() const { return count; }
};

constexpr bool constexpr_copying() {
  int values[4] = {10, 20, 30, 40};
  fixed_range<int, 4> range(values);
  using vec4 = std::simd::vec<int, 4>;
  vec4 v(range);
  vec4::mask_type mask([](auto i) { return i == 0 || i == 2; });
  vec4 masked(range, mask);
  return v[0] == 10 && v[3] == 40 && masked[0] == 10 && masked[1] == 0 &&
         masked[2] == 30 && masked[3] == 0;
}

static_assert(constexpr_copying());

int main(int, char**) {
  using vec4 = std::simd::vec<int, 4>;
  using mask4 = vec4::mask_type;

  static_assert(constructible_from_range<vec4, int (&)[4]>);
  static_assert(!constructible_from_range<vec4, int (&)[3]>);
  static_assert(masked_constructible_from_range<vec4, int (&)[4]>);
  static_assert(!masked_constructible_from_range<vec4, int (&)[3]>);

  static_assert(constructible_from_range<vec4, std::span<int, 4>>);
  static_assert(!constructible_from_range<vec4, std::span<int, 3>>);
  static_assert(!constructible_from_range<vec4, std::span<int>>);
  static_assert(masked_constructible_from_range<vec4, std::span<int, 4>>);
  static_assert(!masked_constructible_from_range<vec4, std::span<int, 3>>);
  static_assert(!masked_constructible_from_range<vec4, std::span<int>>);

  static_assert(!constructible_from_range<vec4, std::vector<int>&>);
  static_assert(!masked_constructible_from_range<vec4, std::vector<int>&>);
  static_assert(!constructible_from_range<vec4, runtime_range<int>&>);
  static_assert(!masked_constructible_from_range<vec4, runtime_range<int>&>);
  static_assert(constructible_from_range<vec4, fixed_range<int, 4>&>);
  static_assert(masked_constructible_from_range<vec4, fixed_range<int, 4>&>);

  static_assert(deducible_from_range<int (&)[4]>);
  static_assert(deducible_from_range<int (&)[3]>);
  static_assert(deducible_from_range<std::span<int, 4>>);
  static_assert(deducible_from_range<std::span<int, 3>>);
  static_assert(!deducible_from_range<std::span<int>>);
  static_assert(!deducible_from_range<std::vector<int>&>);
  static_assert(!deducible_from_range<runtime_range<int>&>);
  static_assert(deducible_from_range<fixed_range<int, 4>&>);

  int values[4] = {1, 2, 3, 4};
  vec4 v(values);
  mask4 mask([](auto i) { return i == 1 || i == 3; });
  vec4 masked(values, mask);
  assert(v[0] == 1 && v[3] == 4);
  assert(masked[0] == 0 && masked[1] == 2 && masked[2] == 0 && masked[3] == 4);

  std::span<int, 4> fixed_span(values);
  vec4 from_span(fixed_span);
  vec4 masked_from_span(fixed_span, mask);
  assert(from_span[1] == 2 && from_span[2] == 3);
  assert(masked_from_span[0] == 0 && masked_from_span[1] == 2 && masked_from_span[3] == 4);

  fixed_range<int, 4> range(values);
  vec4 from_custom(range);
  vec4 masked_from_custom(range, mask);
  assert(from_custom[0] == 1 && from_custom[3] == 4);
  assert(masked_from_custom[0] == 0 && masked_from_custom[1] == 2 && masked_from_custom[3] == 4);

  auto deduced = std::simd::basic_vec(fixed_span);
  static_assert(std::same_as<decltype(deduced), vec4>);
  assert(deduced[0] == 1 && deduced[3] == 4);
}
