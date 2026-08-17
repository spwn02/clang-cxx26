//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// Forward/backward traversal agreement, iterator comparisons (hive
// iterators model three_way_comparable<strong_ordering> even though the
// container itself is only bidirectional), across multiple blocks and with
// erased runs skipped correctly in both directions.

#include <hive>
#include <cassert>
#include <vector>
#include <algorithm>
#include <compare>
#include <concepts>

int main(int, char**) {
  static_assert(std::bidirectional_iterator<std::hive<int>::iterator>);
  static_assert(std::three_way_comparable<std::hive<int>::iterator, std::strong_ordering>);

  // Force multiple blocks with a small fixed block size.
  std::hive<int> h(std::hive_limits(4, 4));
  std::vector<std::hive<int>::iterator> its;
  for (int i = 0; i < 40; ++i)
    its.push_back(h.insert(i));

  // Erase a scattered subset, including whole blocks' worth of adjacent
  // elements (to exercise skipblock merging across a wide range).
  for (int i = 0; i < 40; ++i)
    if (i % 3 == 0 || (i >= 20 && i < 24))
      h.erase(its[i]);

  std::vector<int> fwd(h.begin(), h.end());
  std::vector<int> bwd;
  for (auto it = h.end(); it != h.begin();) {
    --it;
    bwd.push_back(*it);
  }
  std::reverse(bwd.begin(), bwd.end());
  assert(fwd == bwd);

  // begin()/end() on an empty hive.
  {
    std::hive<int> e;
    assert(e.begin() == e.end());
  }

  // Iterator ordering is consistent with traversal order.
  {
    auto it = h.begin();
    auto prev = it;
    ++it;
    for (; it != h.end(); ++it, ++prev) {
      assert(prev < it);
      assert((prev <=> it) == std::strong_ordering::less);
    }
  }

  // const_iterator conversion.
  {
    std::hive<int>::const_iterator cit = h.begin();
    assert(cit == h.begin());
  }

  return 0;
}
