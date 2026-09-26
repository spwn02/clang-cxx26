//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <ranges>

// LWG 3555: transform_view::iterator::iterator_concept is computed from the (possibly const-qualified)
// underlying range that the iterator iterates over.

#include <cstddef>
#include <iterator>
#include <ranges>

// A forward-only iterator over ints.
struct ForwardIterator {
  using value_type      = int;
  using difference_type = std::ptrdiff_t;
  int* p_               = nullptr;
  constexpr int& operator*() const { return *p_; }
  constexpr ForwardIterator& operator++() { ++p_; return *this; }
  constexpr ForwardIterator operator++(int) { auto t = *this; ++p_; return t; }
  friend constexpr bool operator==(const ForwardIterator&, const ForwardIterator&) = default;
};
static_assert(std::forward_iterator<ForwardIterator> && !std::bidirectional_iterator<ForwardIterator>);

// Non-const iteration is random access, const iteration is only forward.
struct View : std::ranges::view_base {
  int* data_ = nullptr;
  std::size_t size_ = 0;
  constexpr int* begin() { return data_; }
  constexpr int* end() { return data_ + size_; }
  constexpr ForwardIterator begin() const { return ForwardIterator{data_}; }
  constexpr ForwardIterator end() const { return ForwardIterator{data_ + size_}; }
};
static_assert(std::ranges::random_access_range<View>);
static_assert(std::ranges::forward_range<const View> && !std::ranges::bidirectional_range<const View>);

struct Identity {
  constexpr int operator()(int x) const { return x; }
};

using TV = std::ranges::transform_view<View, Identity>;
using NonConstIterator = std::ranges::iterator_t<TV>;
using ConstIterator    = std::ranges::iterator_t<const TV>;

static_assert(std::same_as<NonConstIterator::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::same_as<ConstIterator::iterator_concept, std::forward_iterator_tag>);
static_assert(std::random_access_iterator<NonConstIterator>);
static_assert(!std::random_access_iterator<ConstIterator> && std::forward_iterator<ConstIterator>);

int main(int, char**) { return 0; }
