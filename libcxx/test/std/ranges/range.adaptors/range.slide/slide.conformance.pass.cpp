//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-ranges

// [range.slide]: slide_view follows the three caching strategies of [range.slide.view],
// the iterator and sentinel of [range.slide.iterator] / [range.slide.sentinel].

#include <algorithm>
#include <cassert>
#include <concepts>
#include <forward_list>
#include <list>
#include <ranges>
#include <span>
#include <vector>

namespace r = std::ranges;
namespace v = std::views;

template <class T>
concept const_iterable = requires(const T& x) { x.begin(); x.end(); };

constexpr bool test() {
  // [range.slide.overview] example: prints [1, 2] [2, 3] [3, 4]
  {
    std::vector vec = {1, 2, 3, 4};
    int expected[][2] = {{1, 2}, {2, 3}, {3, 4}};
    int k = 0;
    for (auto i : vec | v::slide(2)) {
      assert(i[0] == expected[k][0] && i[1] == expected[k][1]);
      ++k;
    }
    assert(k == 3);
  }
  // [range.slide.view] size: four elements with a window of three -> two windows, end - begin == size,
  // and the first element of the final window is 2; an oversized window gives an empty view.
  {
    int a[] = {1, 2, 3, 4};
    auto x = v::slide(a, 3);
    assert(x.size() == 2);
    assert(x.end() - x.begin() == 2);
    assert((*--x.end())[0] == 2);
    int b[] = {1, 2};
    auto y = v::slide(b, 3);
    assert(y.size() == 0 && r::distance(y.begin(), y.end()) == 0);
    assert(r::empty(y));
  }
  // Reversing yields the windows in reverse order, each with the full window size.
  {
    int a[] = {1, 2, 3, 4, 5};
    int firsts[3];
    int k = 0;
    for (auto w : v::slide(a, 3) | v::reverse) {
      assert(r::size(w) == 3);
      firsts[k++] = w[0];
    }
    assert(k == 3 && firsts[0] == 3 && firsts[1] == 2 && firsts[2] == 1);
  }
  // [range.slide.iterator]: value_type / reference are those of views::counted (span for contiguous bases).
  {
    using V = decltype(v::slide(std::declval<int (&)[4]>(), 3));
    static_assert(std::same_as<r::range_reference_t<V>, std::span<int>>);
    static_assert(std::same_as<r::range_value_t<V>, std::span<int>>);
  }
  // slide-caches-last: bidirectional + common but not random-access/sized: no const begin/end.
  {
    std::list<int> l{1, 2, 3, 4};
    auto s = l | v::slide(2);
    static_assert(!const_iterable<decltype(s)>);
    static_assert(std::bidirectional_iterator<decltype(s.begin())>);
    static_assert(r::common_range<decltype(s)>);
    assert(std::distance(s.begin(), s.end()) == 3);
    assert(s.begin() == s.begin());
    int sum = 0;
    for (auto w : s | v::reverse)
      for (int e : w)
        sum += e;
    assert(sum == 3 + 5 + 7);
  }
  // slide-caches-first: forward-only base.
  {
    std::forward_list<int> l{1, 2, 3, 4};
    auto s = l | v::slide(3);
    static_assert(!const_iterable<decltype(s)>);
    static_assert(std::forward_iterator<decltype(s.begin())>);
    assert(std::distance(s.begin(), s.end()) == 2);
    auto b1 = s.begin();
    auto b2 = s.begin(); // repeated begin() is cached
    assert(b1 == b2);
  }
  // Non-common base: sentinel with sized subtraction when the base supports it.
  {
    auto iota = r::iota_view(0, 6) | v::take_while([](int i) { return i < 5; }); // not sized, not common
    auto s    = iota | v::slide(2);
    static_assert(!r::common_range<decltype(s)>);
    int n = 0;
    for (auto w : s) {
      assert(w[1] == w[0] + 1);
      ++n;
    }
    assert(n == 4);
    auto ss = r::subrange(std::counted_iterator(r::begin(iota), 0), std::default_sentinel); // keep ADL honest
    (void)ss;
  }
  // [range.slide.view]: the class has no default constructor.
  static_assert(!std::default_initializable<r::slide_view<r::ref_view<std::vector<int>>>>);
  // slide-caches-nothing: random-access + sized base gives a const-iterable view.
  {
    std::vector<int> vec{1, 2, 3, 4, 5};
    const auto s = vec | v::slide(2);
    static_assert(const_iterable<decltype(s)>);
    assert(s.size() == 4 && (s.end() - s.begin()) == 4);
  }
  return true;
}

int main(int, char**) {
  test();
#if __cplusplus > 202302L // std::list and std::forward_list are constexpr only in C++26
  static_assert(test());
#endif
  return 0;
}
