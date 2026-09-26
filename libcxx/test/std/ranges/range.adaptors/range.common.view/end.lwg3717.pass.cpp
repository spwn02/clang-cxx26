//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <ranges>

// LWG 3717: common_view::end() in the random_access_range && sized_range case adds the size converted to
// range_difference_t<V> to the beginning iterator.

#include <cassert>
#include <cstddef>
#include <iterator>
#include <ranges>

// A random access iterator whose operator+ only accepts exactly its difference_type: an unconverted size_t
// argument would select the deleted overload template.
struct Iterator {
  using value_type        = int;
  using difference_type   = std::ptrdiff_t;
  using iterator_concept  = std::random_access_iterator_tag;
  int* p_                 = nullptr;

  constexpr int& operator*() const { return *p_; }
  constexpr int& operator[](difference_type n) const { return p_[n]; }
  constexpr Iterator& operator++() { ++p_; return *this; }
  constexpr Iterator operator++(int) { auto t = *this; ++p_; return t; }
  constexpr Iterator& operator--() { --p_; return *this; }
  constexpr Iterator operator--(int) { auto t = *this; --p_; return t; }
  constexpr Iterator& operator+=(difference_type n) { p_ += n; return *this; }
  constexpr Iterator& operator-=(difference_type n) { p_ -= n; return *this; }
  friend constexpr Iterator operator+(Iterator it, difference_type n) { return it += n; }
  template <class T>
  friend constexpr Iterator operator+(Iterator, T) = delete;
  friend constexpr Iterator operator+(difference_type n, Iterator it) { return it += n; }
  friend constexpr Iterator operator-(Iterator it, difference_type n) { return it -= n; }
  friend constexpr difference_type operator-(Iterator a, Iterator b) { return a.p_ - b.p_; }
  friend constexpr bool operator==(Iterator, Iterator) = default;
  friend constexpr auto operator<=>(Iterator a, Iterator b) { return a.p_ <=> b.p_; }
};
static_assert(std::random_access_iterator<Iterator>);

// A sentinel type different from the iterator, so that the view is not a common_range.
struct Sentinel {
  int* p_;
  friend constexpr bool operator==(Iterator it, Sentinel s) { return it.p_ == s.p_; }
  friend constexpr Iterator::difference_type operator-(Iterator it, Sentinel s) { return it.p_ - s.p_; }
  friend constexpr Iterator::difference_type operator-(Sentinel s, Iterator it) { return s.p_ - it.p_; }
};
static_assert(std::sized_sentinel_for<Sentinel, Iterator>);

struct View : std::ranges::view_base {
  int* data_;
  std::size_t size_;
  constexpr Iterator begin() const { return Iterator{data_}; }
  constexpr Sentinel end() const { return Sentinel{data_ + size_}; }
  constexpr std::size_t size() const { return size_; } // unsigned, unlike difference_type
};
static_assert(std::ranges::random_access_range<View> && std::ranges::sized_range<View>);

constexpr bool test() {
  int a[] = {1, 2, 3, 4};
  {
    std::ranges::common_view<View> view(View{{}, a, 4});
    auto e = view.end();
    assert(e - view.begin() == 4);
    static_assert(std::same_as<decltype(e), Iterator>);
  }
  {
    const std::ranges::common_view<View> view(View{{}, a, 3});
    auto e = view.end();
    assert(e - view.begin() == 3);
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
