//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// std::ranges::concat_view, std::views::concat

#include <ranges>

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <iterator>
#include <list>
#include <vector>
#include "test_macros.h"

// Single-argument case: `views::concat(r)` is exactly `views::all(r)`, not a `concat_view`.
constexpr bool testSingleArgumentPassthrough() {
  std::vector<int> v = {1, 2, 3};
  std::same_as<std::ranges::ref_view<std::vector<int>>> decltype(auto) result = std::views::concat(v);
  assert(std::ranges::equal(result, v));
  return true;
}

// Two random_access, common, sized ranges concatenate into a random_access, common, sized view.
constexpr bool testRandomAccess() {
  std::array a{1, 2, 3};
  std::array b{4, 5, 6, 7};

  auto v = std::views::concat(a, b);
  static_assert(std::ranges::random_access_range<decltype(v)>);
  static_assert(std::ranges::common_range<decltype(v)>);
  static_assert(std::ranges::sized_range<decltype(v)>);

  assert(v.size() == 7);
  int expected[] = {1, 2, 3, 4, 5, 6, 7};
  assert(std::ranges::equal(v, expected));

  // Indexing and arithmetic cross the boundary between the two underlying ranges.
  assert(v[0] == 1);
  assert(v[2] == 3);
  assert(v[3] == 4);
  assert(v[6] == 7);

  auto first = v.begin();
  auto last  = v.begin() + 7;
  assert(last == v.end());
  assert(last - first == 7);

  auto it = first + 3;
  assert(*it == 4);
  --it;
  assert(*it == 3); // crossed back into the first range
  it -= 2;
  assert(it == first);

  return true;
}

// bidirectional_iterator underlying ranges yield a bidirectional (not random-access) concat_view.
constexpr bool testBidirectional() {
  std::list<int> l1 = {1, 2};
  std::list<int> l2 = {3, 4, 5};

  auto v = std::views::concat(l1, l2);
  static_assert(std::ranges::bidirectional_range<decltype(v)>);
  static_assert(!std::ranges::random_access_range<decltype(v)>);

  int expected[] = {1, 2, 3, 4, 5};
  assert(std::ranges::equal(v, expected));

  auto it = v.end();
  --it;
  assert(*it == 5);
  --it;
  --it;
  assert(*it == 3);
  --it; // crosses back into l1
  assert(*it == 2);

  return true;
}

// A forward iterator with a distinct sentinel type (genuinely non-common, unlike e.g.
// `forward_list` whose begin()/end() share one iterator type).
struct NonCommonForwardIterator {
  using value_type       = int;
  using difference_type  = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;

  int* p_ = nullptr;

  constexpr int operator*() const { return *p_; }
  constexpr NonCommonForwardIterator& operator++() {
    ++p_;
    return *this;
  }
  constexpr NonCommonForwardIterator operator++(int) {
    auto tmp = *this;
    ++*this;
    return tmp;
  }
  friend constexpr bool operator==(const NonCommonForwardIterator&, const NonCommonForwardIterator&) = default;
};
struct NonCommonForwardSentinel {
  int* end_ = nullptr;
  friend constexpr bool operator==(const NonCommonForwardIterator& it, const NonCommonForwardSentinel& s) {
    return it.p_ == s.end_;
  }
};
static_assert(std::forward_iterator<NonCommonForwardIterator>);

struct NonCommonForwardRange {
  int* data_;
  int size_;
  constexpr NonCommonForwardIterator begin() const { return NonCommonForwardIterator{data_}; }
  constexpr NonCommonForwardSentinel end() const { return NonCommonForwardSentinel{data_ + size_}; }
};
static_assert(std::ranges::forward_range<NonCommonForwardRange>);
static_assert(!std::ranges::common_range<NonCommonForwardRange>);

// A trailing non-common forward range makes the whole concat_view non-common too -- verify
// `end()` degrades to `default_sentinel` and iteration still works.
constexpr bool testForwardNonCommon() {
  std::vector<int> a = {1, 2};
  int data[] = {3, 4, 5};
  NonCommonForwardRange b{data, 3};

  auto v = std::views::concat(a, b);
  static_assert(std::ranges::forward_range<decltype(v)>);
  static_assert(!std::ranges::bidirectional_range<decltype(v)>);
  static_assert(!std::ranges::common_range<decltype(v)>);
  static_assert(std::same_as<decltype(v.end()), std::default_sentinel_t>);

  int expected[] = {1, 2, 3, 4, 5};
  assert(std::ranges::equal(v, expected));
  return true;
}

// Heterogeneous element types resolve through common_reference/common_type.
constexpr bool testHeterogeneousTypes() {
  std::array<int, 2> a{1, 2};
  std::array<short, 2> b{3, 4};

  auto v = std::views::concat(a, b);
  static_assert(std::same_as<std::ranges::range_value_t<decltype(v)>, int>);
  int expected[] = {1, 2, 3, 4};
  assert(std::ranges::equal(v, expected));
  return true;
}

// Three-range concat exercises the recursive __satisfy/__prev machinery beyond a single boundary.
constexpr bool testThreeRanges() {
  std::array a{1, 2};
  std::array b{3};
  std::array c{4, 5, 6};

  auto v = std::views::concat(a, b, c);
  int expected[] = {1, 2, 3, 4, 5, 6};
  assert(std::ranges::equal(v, expected));
  assert(v.size() == 6);

  auto it = v.begin();
  std::ranges::advance(it, 2); // lands exactly on the single-element middle range
  assert(*it == 3);
  ++it; // crosses into the third range
  assert(*it == 4);

  return true;
}

int main(int, char**) {
  testSingleArgumentPassthrough();
  static_assert(testSingleArgumentPassthrough());

  testRandomAccess();
  static_assert(testRandomAccess());

  testBidirectional();
  static_assert(testBidirectional());

  testForwardNonCommon();
  static_assert(testForwardNonCommon());

  testHeterogeneousTypes();
  static_assert(testHeterogeneousTypes());

  testThreeRanges();
  static_assert(testThreeRanges());

  // CTAD.
  {
    std::vector<int> a = {1, 2};
    std::vector<int> b = {3, 4};
    std::ranges::concat_view view(a, b);
    static_assert(
        std::same_as<decltype(view),
                     std::ranges::concat_view<std::ranges::ref_view<std::vector<int>>, std::ranges::ref_view<std::vector<int>>>>);
  }

  return 0;
}
