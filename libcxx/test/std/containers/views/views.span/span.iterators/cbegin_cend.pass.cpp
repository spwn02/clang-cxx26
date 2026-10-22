//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <span>

// using const_iterator = std::const_iterator<iterator>;
// using const_reverse_iterator = std::const_iterator<reverse_iterator>;
// constexpr const_iterator cbegin() const noexcept;
// constexpr const_iterator cend() const noexcept;
// constexpr const_reverse_iterator crbegin() const noexcept;
// constexpr const_reverse_iterator crend() const noexcept;

#include <cassert>
#include <iterator>
#include <span>
#include <type_traits>

#include "test_macros.h"

template <class Span>
constexpr void check(Span s, typename Span::element_type* data, int n) {
  static_assert(std::is_same_v<typename Span::const_iterator, std::const_iterator<typename Span::iterator>>);
  static_assert(
      std::is_same_v<typename Span::const_reverse_iterator, std::const_iterator<typename Span::reverse_iterator>>);
  static_assert(noexcept(s.cbegin()) && noexcept(s.cend()) && noexcept(s.crbegin()) && noexcept(s.crend()));
  static_assert(std::is_same_v<std::iter_reference_t<typename Span::const_iterator>, const typename Span::element_type&>);
  assert(s.cend() - s.cbegin() == n);
  assert(s.crend() - s.crbegin() == n);
  if (n != 0) {
    assert(&*s.cbegin() == data);
    assert(&*s.crbegin() == data + n - 1);
  }
}

constexpr bool test() {
  int a[3] = {1, 2, 3};
  check(std::span<int>(a), a, 3);
  check(std::span<const int>(a), a, 3);
  check(std::span<int, 3>(a), a, 3);
  check(std::span<int>(), nullptr, 0);
  // A span of const elements already has a constant iterator.
  static_assert(std::is_same_v<std::span<const int>::const_iterator, std::span<const int>::iterator>);
  static_assert(!std::is_same_v<std::span<int>::const_iterator, std::span<int>::iterator>);
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
