//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <generator>

// generator::promise_type::yield_value(ranges::elements_of<R, Alloc>)
//   Constraints: convertible_to<ranges::range_reference_t<R>, yielded> is true.

#include <concepts>
#include <generator>
#include <ranges>
#include <vector>

template <class Gen, class Range>
concept can_yield_elements_of = requires(typename Gen::promise_type& p, Range&& r) {
  p.yield_value(std::ranges::elements_of(std::forward<Range>(r)));
};

// yielded is int&& for generator<int>; int& is not convertible to int&&.
static_assert(!can_yield_elements_of<std::generator<int>, std::vector<int>&>);
static_assert(can_yield_elements_of<std::generator<const int&>, std::vector<int>&>);
static_assert(can_yield_elements_of<std::generator<int&>, std::vector<int>&>);
static_assert(!can_yield_elements_of<std::generator<int&&>, std::vector<int>>);
