//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// Basic functional coverage of std::hive: insert/erase with iterator
// stability, permutation-based equality, sort/unique/splice.

#include <hive>
#include <cassert>

int main(int, char**) {
  {
    std::hive<int> h;
    auto it1 = h.insert(1);
    auto it2 = h.insert(2);
    auto it3 = h.insert(3);
    assert(h.size() == 3);

    h.erase(it2);
    assert(h.size() == 2);
    // Erasing one element must not invalidate iterators to the others.
    assert(*it1 == 1 && *it3 == 3);
  }

  {
    std::hive<int> a{3, 1, 2};
    std::hive<int> b{1, 2, 3};
    assert(a == b);

    std::hive<int> c{1, 2};
    assert(a != c);
  }

  {
    std::hive<int> h;
    h.insert({5, 4, 4, 3});
    h.sort();
    auto n = h.unique();
    assert(n == 1);
    assert(h.size() == 3);
  }

  {
    std::hive<int> a{1, 2};
    std::hive<int> b{3, 4};
    a.splice(b);
    assert(a.size() == 4 && b.size() == 0);
  }

  return 0;
}
