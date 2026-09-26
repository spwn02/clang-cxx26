//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <ranges>

// template<input_range R> using const_iterator_t = const_iterator<iterator_t<R>>;
// template<range R> using const_sentinel_t = const_sentinel<sentinel_t<R>>;     (LWG 3770)
// template<input_range R> using range_const_reference_t = iter_const_reference_t<iterator_t<R>>;

#include <array>
#include <concepts>
#include <iterator>
#include <ranges>
#include <vector>

// int* is not a constant iterator, so it is wrapped (ranges::cbegin(array) returns const int* because it applies
// possibly-const-range first, but const_iterator_t<R> starts from iterator_t<R>).
static_assert(std::same_as<std::ranges::const_iterator_t<int[3]>, std::basic_const_iterator<int*>>);
static_assert(std::same_as<std::ranges::const_sentinel_t<int[3]>, std::basic_const_iterator<int*>>);
static_assert(std::same_as<std::ranges::const_iterator_t<int (&)[3]>, std::basic_const_iterator<int*>>);
static_assert(std::same_as<std::ranges::const_sentinel_t<int (&)[3]>, std::basic_const_iterator<int*>>);
static_assert(std::same_as<std::ranges::const_iterator_t<const int[3]>, const int*>);
static_assert(std::same_as<std::ranges::const_sentinel_t<const int[3]>, const int*>);
static_assert(std::same_as<std::ranges::range_const_reference_t<int[3]>, const int&>);

static_assert(std::same_as<std::ranges::const_iterator_t<std::vector<int>>,
                           std::basic_const_iterator<std::vector<int>::iterator>>);
static_assert(std::same_as<std::ranges::const_sentinel_t<std::vector<int>>,
                           std::basic_const_iterator<std::vector<int>::iterator>>);
static_assert(std::same_as<std::ranges::const_iterator_t<const std::vector<int>>, std::vector<int>::const_iterator>);
static_assert(std::same_as<std::ranges::range_const_reference_t<std::vector<int>>, const int&>);
static_assert(std::same_as<std::ranges::range_const_reference_t<const std::vector<int>>, const int&>);

// An iterator whose reference is already a prvalue value is constant and is not wrapped.
using Iota = std::ranges::iota_view<int, int>;
static_assert(std::same_as<std::ranges::const_iterator_t<Iota>, std::ranges::iterator_t<Iota>>);
static_assert(std::same_as<std::ranges::range_const_reference_t<Iota>, int>);
using Unbounded = std::ranges::iota_view<int>;
static_assert(std::same_as<std::ranges::const_sentinel_t<Unbounded>, std::unreachable_sentinel_t>);

// A range that is not an input range has no const_iterator_t, but a sentinel type may still exist.
struct NotInput {
  int* begin();
  int* end();
};
template <class R>
concept has_const_iterator_t = requires { typename std::ranges::const_iterator_t<R>; };
static_assert(has_const_iterator_t<NotInput>);
static_assert(!has_const_iterator_t<int>);
