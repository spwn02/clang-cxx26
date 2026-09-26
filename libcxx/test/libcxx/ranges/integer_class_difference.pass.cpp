//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// P2393R1: ranges and their adaptors work with integer-class types as difference and size types.

#include <cassert>
#include <concepts>
#include <iterator>
#include <ranges>

#include "integer_class.h"
#include "test_macros.h"

#ifdef __SIZEOF_INT128__

using S = integer_class::Signed;
using U = integer_class::Unsigned;

// A contiguous-like random access iterator over ints whose difference type is a signed integer-class type.
struct It {
  using difference_type   = S;
  using value_type        = int;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept  = std::random_access_iterator_tag;

  int* p = nullptr;

  constexpr It() = default;
  constexpr explicit It(int* q) : p(q) {}
  constexpr int& operator*() const { return *p; }
  constexpr int& operator[](S n) const { return p[static_cast<long>(n)]; }
  constexpr It& operator++() { ++p; return *this; }
  constexpr It operator++(int) { auto t = *this; ++p; return t; }
  constexpr It& operator--() { --p; return *this; }
  constexpr It operator--(int) { auto t = *this; --p; return t; }
  constexpr It& operator+=(S n) { p += static_cast<long>(n); return *this; }
  constexpr It& operator-=(S n) { p -= static_cast<long>(n); return *this; }
  friend constexpr It operator+(It i, S n) { return i += n; }
  friend constexpr It operator+(S n, It i) { return i += n; }
  friend constexpr It operator-(It i, S n) { return i -= n; }
  friend constexpr S operator-(It a, It b) { return S(static_cast<long>(a.p - b.p)); }
  friend constexpr bool operator==(It a, It b) { return a.p == b.p; }
  friend constexpr auto operator<=>(It a, It b) { return a.p <=> b.p; }
};

static_assert(std::random_access_iterator<It>);
static_assert(std::__iterator_traits_detail::__cpp17_input_iterator<It>); // LWG3670: the difference type may be an integer-class type
static_assert(std::same_as<std::iter_difference_t<It>, S>);
static_assert(std::sized_sentinel_for<It, It>);

using Sub = std::ranges::subrange<It, It, std::ranges::subrange_kind::sized>;
static_assert(std::ranges::random_access_range<Sub>);
static_assert(std::ranges::sized_range<Sub>);
static_assert(std::same_as<std::ranges::range_difference_t<Sub>, S>);

// A range whose size() returns an integer-class type.
struct SizedRange {
  int data[4] = {10, 20, 30, 40};
  constexpr int* begin() { return data; }
  constexpr int* end() { return data + 4; }
  constexpr U size() const { return U(4); }
};

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};

  // ranges::size and ranges::ssize
  SizedRange r;
  static_assert(std::same_as<decltype(std::ranges::size(r)), U>);
  assert(std::ranges::size(r) == U(4));
  static_assert(std::same_as<decltype(std::ranges::ssize(r)), S>);
  assert(std::ranges::ssize(r) == S(4));
  static_assert(std::ranges::sized_range<SizedRange>);

  // subrange
  Sub s(It(a), It(a + 5), U(5));
  static_assert(std::same_as<decltype(s.size()), U>);
  assert(s.size() == U(5));
  assert(std::ranges::size(s) == U(5));
  assert(std::ranges::ssize(s) == S(5));
  assert(std::ranges::distance(s) == S(5));
  Sub s2(It(a), It(a + 5), U(5));
  s2.advance(S(2));
  assert(s2.size() == U(3));
  assert(*s2.begin() == 3);
  s2.advance(S(-1));
  assert(s2.size() == U(4));

  // take / drop / counted with integer-class counts
  auto t = s | std::views::take(S(3));
  assert(std::ranges::size(t) == U(3));
  assert(std::ranges::distance(t) == S(3));
  auto d = s | std::views::drop(S(2));
  assert(std::ranges::size(d) == U(3));
  assert(*d.begin() == 3);
  auto d2 = s | std::views::drop(S(10));
  assert(std::ranges::size(d2) == U(0));
  auto t2 = s | std::views::take(S(10)); // more than the range has: clamped
  assert(std::ranges::size(t2) == U(5));

  auto c = std::views::counted(It(a), S(4));
  assert(std::ranges::distance(c) == S(4));
  auto c2 = std::views::counted(It(a), 4); // an int converts to the integer-class difference type
  assert(std::ranges::distance(c2) == S(4));

  // repeat_view with an integer-class bound
  auto rep = std::views::repeat(7, S(3));
  static_assert(std::same_as<decltype(rep.size()), U>);
  assert(rep.size() == U(3));
  assert(std::ranges::distance(rep) == 3);

  // counted_iterator and common_iterator
  using CT = std::counted_iterator<It>;
  using CI = std::common_iterator<CT, std::default_sentinel_t>;
  static_assert(std::same_as<std::iter_difference_t<CI>, S>);
  CT ct(It(a), S(3));
  assert(ct.count() == S(3));
  assert((ct + S(1)).count() == S(2));
  assert(std::default_sentinel - ct == S(3));
  assert(ct - std::default_sentinel == S(-3));
  assert(CI(ct) - CI(CT(It(a), S(1))) == S(-2));

  // iterator arithmetic with the integer-class difference type
  It i(a);
  assert(*(i + S(2)) == 3);
  assert(i[S(4)] == 5);
  assert(std::ranges::next(i, S(3)) == It(a + 3));
  assert(std::ranges::next(i, S(9), It(a + 5)) == It(a + 5));
  assert(std::ranges::advance(i, S(9), It(a + 5)) == S(4));

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}

#else
int main(int, char**) { return 0; }
#endif
