//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <ranges>

// template<range R> using const_iterator_t = decltype(ranges::cbegin(declval<R&>()));
// template<range R> using const_sentinel_t = decltype(ranges::cend(declval<R&>()));
// template<range R> using range_const_reference_t = iter_const_reference_t<iterator_t<R>>;

#include <array>
#include <concepts>
#include <iterator>
#include <ranges>
#include <vector>

// [ranges.syn]: the aliases name the result of ranges::cbegin / ranges::cend, which apply
// possibly-const-range first (so an array of int yields const int*).
static_assert(std::same_as<std::ranges::const_iterator_t<int[3]>, const int*>);
static_assert(std::same_as<std::ranges::const_sentinel_t<int[3]>, const int*>);
static_assert(std::same_as<std::ranges::const_iterator_t<int (&)[3]>, const int*>);
static_assert(std::same_as<std::ranges::const_sentinel_t<int (&)[3]>, const int*>);
static_assert(std::same_as<std::ranges::const_iterator_t<const int[3]>, const int*>);
static_assert(std::same_as<std::ranges::const_sentinel_t<const int[3]>, const int*>);
static_assert(std::same_as<std::ranges::range_const_reference_t<int[3]>, const int&>);

static_assert(std::same_as<std::ranges::const_iterator_t<std::vector<int>>, std::vector<int>::const_iterator>);
static_assert(std::same_as<std::ranges::const_sentinel_t<std::vector<int>>, std::vector<int>::const_iterator>);
static_assert(std::same_as<std::ranges::const_iterator_t<const std::vector<int>>, std::vector<int>::const_iterator>);
static_assert(std::same_as<std::ranges::range_const_reference_t<std::vector<int>>, const int&>);
static_assert(std::same_as<std::ranges::range_const_reference_t<const std::vector<int>>, const int&>);

// An iterator whose reference is already a prvalue value is constant and is not wrapped.
using Iota = std::ranges::iota_view<int, int>;
static_assert(std::same_as<std::ranges::const_iterator_t<Iota>, std::ranges::iterator_t<Iota>>);
static_assert(std::same_as<std::ranges::range_const_reference_t<Iota>, int>);
using Unbounded = std::ranges::iota_view<int>;
static_assert(std::same_as<std::ranges::const_sentinel_t<Unbounded>, std::unreachable_sentinel_t>);

// A range that is not an input range still has the aliases (they only need a range).
struct NotInput {
  int* begin();
  int* end();
};
template <class R>
concept has_const_iterator_t = requires { typename std::ranges::const_iterator_t<R>; };
static_assert(has_const_iterator_t<NotInput>);
static_assert(!has_const_iterator_t<int>);
