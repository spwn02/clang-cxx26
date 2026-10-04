//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <iterator>

// [const.iterators.iterator]: basic_const_iterator<Iterator> converts to a constant iterator CI that
// Iterator const& (lvalue) or Iterator (rvalue) converts to.

#include <cassert>
#include <concepts>
#include <iterator>
#include <utility>

constexpr bool test() {
  int a[2]{};
  std::basic_const_iterator<int*> i(a);
  static_assert(std::convertible_to<std::basic_const_iterator<int*>, const int*>);
  static_assert(std::convertible_to<const std::basic_const_iterator<int*>&, const int*>);
  const int* p = i; // const& overload
  assert(p == a);
  const int* q = std::move(i); // && overload
  assert(q == a);
  // A non-constant target is not accepted.
  static_assert(!std::convertible_to<std::basic_const_iterator<int*>, int*>);
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
