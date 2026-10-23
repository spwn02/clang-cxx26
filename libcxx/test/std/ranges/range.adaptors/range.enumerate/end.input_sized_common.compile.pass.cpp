//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// LWG3919: enumerate_view::end() returns an iterator only if the base is
// forward, common and sized.

#include <concepts>
#include <iterator>
#include <ranges>

struct InputIt {
  using difference_type = int;
  using value_type      = int;
  int operator*() const;
  InputIt& operator++();
  void operator++(int);
  bool operator==(const InputIt&) const;
};

struct SizedCommonInput : std::ranges::view_base {
  InputIt begin() const;
  InputIt end() const;
  unsigned size() const;
};

static_assert(std::ranges::common_range<SizedCommonInput>);
static_assert(std::ranges::sized_range<SizedCommonInput>);
static_assert(!std::ranges::forward_range<SizedCommonInput>);

using E = std::ranges::enumerate_view<SizedCommonInput>;
static_assert(!std::same_as<std::ranges::iterator_t<E>, std::ranges::sentinel_t<E>>);
static_assert(!std::ranges::common_range<E>);
static_assert(!std::ranges::common_range<const E>);
