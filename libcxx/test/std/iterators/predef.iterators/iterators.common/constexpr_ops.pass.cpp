//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <iterator>

// [common.iter.access], [common.iter.nav]: operator->, prefix ++ and postfix ++ of
// common_iterator are constexpr (the synopsis declares them `constexpr`).

#include <cassert>
#include <iterator>

constexpr bool test() {
  int a[] = {1, 2};
  using C = std::common_iterator<std::counted_iterator<int*>, std::default_sentinel_t>;
  {
    C i(std::counted_iterator(a, 2));
    assert(*i.operator->() == 1);
  }
  {
    C i(std::counted_iterator(a, 2));
    ++i;
    assert(*i == 2);
  }
  {
    C i(std::counted_iterator(a, 2));
    auto old = i++;
    assert(*old == 1 && *i == 2);
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
