//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// Basic functional coverage of std::hive: insert/erase with iterator and
// pointer stability, sort/unique/splice. hive deliberately has no
// operator==/<=> ([hive.overview]p6 -- element order is unspecified), so
// content comparisons here go through a small helper that sorts first.

#include <hive>
#include <cassert>
#include <vector>
#include <algorithm>

template <class T, class Alloc>
std::vector<T> sorted_contents(const std::hive<T, Alloc>& h) {
  std::vector<T> v(h.begin(), h.end());
  std::sort(v.begin(), v.end());
  return v;
}

int main(int, char**) {
  {
    std::hive<int> h;
    auto it1 = h.insert(1);
    auto it2 = h.insert(2);
    auto it3 = h.insert(3);
    assert(h.size() == 3);

    h.erase(it2);
    assert(h.size() == 2);
    // Erasing one element must not invalidate iterators/pointers to others.
    assert(*it1 == 1 && *it3 == 3);
  }

  {
    std::hive<int> a{3, 1, 2};
    std::hive<int> b{1, 2, 3};
    assert(sorted_contents(a) == sorted_contents(b));

    std::hive<int> c{1, 2};
    assert(sorted_contents(a) != sorted_contents(c));
  }

  {
    std::hive<int> h;
    h.insert({5, 4, 4, 3});
    h.sort();
    auto n = h.unique();
    assert(n == 1);
    assert(h.size() == 3);
    assert((sorted_contents(h) == std::vector<int>{3, 4, 5}));
  }

  {
    std::hive<int> a{1, 2};
    std::hive<int> b{3, 4};
    a.splice(b);
    assert(a.size() == 4 && b.size() == 0 && b.empty());
    assert((sorted_contents(a) == std::vector<int>{1, 2, 3, 4}));
  }

  {
    // Pointer stability across many insertions and interleaved erasures.
    std::hive<int> h;
    std::vector<int*> ptrs;
    for (int i = 0; i < 100; ++i)
      ptrs.push_back(&*h.insert(i));
    for (int i = 0; i < 100; i += 2)
      h.erase(h.get_iterator(ptrs[i]));
    for (int i = 1; i < 100; i += 2)
      assert(*ptrs[i] == i);
    assert(h.size() == 50);
  }

  return 0;
}
