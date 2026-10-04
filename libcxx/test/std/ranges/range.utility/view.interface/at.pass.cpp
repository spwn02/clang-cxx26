//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-exceptions
// UNSUPPORTED: no-ranges

// <ranges>

// [view.interface.members]: at(n) returns (*this)[n] and throws out_of_range if n < 0 or
// n >= ranges::distance(derived()); it requires a random-access, sized range.

#include <cassert>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace v = std::views;

template <class T>
concept has_at = requires(T& t) { t.at(0); };

int main(int, char**) {
  std::vector<int> vec{1, 2, 3};
  auto s = vec | v::take(2);
  assert(s.at(1) == 2);
  const auto cs = v::all(vec);
  assert(cs.at(2) == 3);

  bool t1 = false, t2 = false;
  try {
    (void)s.at(2);
  } catch (const std::out_of_range&) {
    t1 = true;
  }
  try {
    (void)s.at(-1);
  } catch (const std::out_of_range&) {
    t2 = true;
  }
  assert(t1 && t2);

  static_assert(has_at<decltype(s)>);
  static_assert(!has_at<decltype(v::iota(0))>); // not sized
  return 0;
}
