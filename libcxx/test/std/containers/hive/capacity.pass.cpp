//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// reserve/capacity/trim_capacity/shrink_to_fit/block_capacity_limits/
// block_capacity_hard_limits/is_within_hard_limits/reshape.

#include <hive>
#include <cassert>
#include <vector>
#include <algorithm>

int main(int, char**) {
  {
    std::hive<int> h;
    h.reserve(100);
    assert(h.capacity() >= 100);
    assert(h.size() == 0);
    assert(h.empty());

    for (int i = 0; i < 50; ++i)
      h.insert(i);
    assert(h.size() == 50);
    size_t cap_with_reserve = h.capacity();

    h.trim_capacity();
    assert(h.size() == 50);
    assert(h.capacity() <= cap_with_reserve);
    assert(h.capacity() >= h.size());
  }

  {
    std::hive<int> h(std::hive_limits(4, 4));
    for (int i = 0; i < 20; ++i)
      h.insert(i);
    // Fragment before shrinking: erase every other element so shrink_to_fit
    // has to deallocate partially-empty groups, not just trim a clean tail.
    for (auto it = h.begin(); it != h.end();) {
      if (*it % 2 == 0)
        it = h.erase(it);
      else
        ++it;
    }
    h.shrink_to_fit();
    assert(h.size() == 10);
    assert(h.capacity() >= h.size());
    std::vector<int> vals(h.begin(), h.end());
    std::sort(vals.begin(), vals.end());
    std::vector<int> expected{1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    assert(vals == expected);
  }

  {
    std::hive<int> h(std::hive_limits(4, 4));
    assert(h.block_capacity_limits().min == 4);
    assert(h.block_capacity_limits().max == 4);

    auto hard = std::hive<int>::block_capacity_hard_limits();
    assert(std::hive<int>::is_within_hard_limits({4, 4}));
    assert(hard.min <= 4 && 4 <= hard.max);

    for (int i = 0; i < 20; ++i)
      h.insert(i);
    assert(h.size() == 20);

    h.reshape(std::hive_limits(64, 64));
    assert(h.block_capacity_limits().min == 64);
    assert(h.size() == 20); // reshape must not lose or duplicate elements
    assert(h.capacity() >= h.size());

    std::vector<int> vals(h.begin(), h.end());
    std::sort(vals.begin(), vals.end());
    for (int i = 0; i < 20; ++i)
      assert(vals[i] == i);

    // Large -> small, on a hive fragmented by prior erasures (reshape
    // migrates via emplace() per element, so it depends on free-list
    // state -- untested by a small->large reshape on a freshly-built hive).
    for (auto it = h.begin(); it != h.end();) {
      if (*it % 3 == 0)
        it = h.erase(it);
      else
        ++it;
    }
    size_t size_before_shrink_reshape = h.size();
    h.reshape(std::hive_limits(4, 4));
    assert(h.block_capacity_limits().min == 4);
    assert(h.size() == size_before_shrink_reshape);
    assert(h.capacity() >= h.size());
    std::vector<int> vals2(h.begin(), h.end());
    std::sort(vals2.begin(), vals2.end());
    std::vector<int> expected2;
    for (int i = 0; i < 20; ++i)
      if (i % 3 != 0)
        expected2.push_back(i);
    assert(vals2 == expected2);
  }

  {
    // max_size / trim_capacity(n)
    std::hive<int> h;
    h.reserve(200);
    h.trim_capacity(50);
    assert(h.capacity() <= 200);
  }

  return 0;
}
