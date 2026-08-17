//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// insert overloads, emplace, assign, from_range_t / deduction guides,
// erase/erase_if, splice failure modes, allocator-extended construction.

#include <hive>
#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>

template <class T, class Alloc>
std::vector<T> sorted_contents(const std::hive<T, Alloc>& h) {
  std::vector<T> v(h.begin(), h.end());
  std::sort(v.begin(), v.end());
  return v;
}

struct Point {
  int x, y;
  bool operator==(const Point&) const = default;
};

int main(int, char**) {
  // emplace
  {
    std::hive<Point> h;
    h.emplace(1, 2);
    h.emplace(3, 4);
    assert(h.size() == 2);
    bool found12 = false, found34 = false;
    for (auto& p : h) {
      if (p == Point{1, 2}) found12 = true;
      if (p == Point{3, 4}) found34 = true;
    }
    assert(found12 && found34);
  }

  // insert(n, value), insert(initializer_list), insert(first,last), insert_range
  {
    std::hive<int> h;
    h.insert(5, 9);
    assert(h.size() == 5);
    for (int v : h)
      assert(v == 9);

    h.insert({1, 2, 3});
    assert(h.size() == 8);

    std::vector<int> more{10, 11};
    h.insert(more.begin(), more.end());
    assert(h.size() == 10);

    h.insert_range(std::array<int, 2>{20, 21});
    assert(h.size() == 12);
  }

  // assign
  {
    std::hive<int> h;
    for (int i = 0; i < 5; ++i)
      h.insert(i);
    h.assign(3, 7);
    assert(h.size() == 3);
    for (int v : h)
      assert(v == 7);

    std::vector<int> src{1, 2, 3, 4};
    h.assign(src.begin(), src.end());
    assert(h.size() == 4);

    h.assign_range(std::array<int, 2>{100, 200});
    assert(h.size() == 2);
  }

  // from_range_t + CTAD deduction guides
  {
    std::array<int, 3> arr{1, 2, 3};
    std::hive h(arr.begin(), arr.end());
    static_assert(std::is_same_v<decltype(h), std::hive<int>>);
    assert(h.size() == 3);

    std::hive h2(std::from_range, std::array<int, 3>{4, 5, 6});
    static_assert(std::is_same_v<decltype(h2), std::hive<int>>);
    assert(h2.size() == 3);
  }

  // erase / erase_if free functions
  {
    std::hive<int> h;
    for (int i = 0; i < 10; ++i)
      h.insert(i);
    auto n = std::erase_if(h, [](int v) { return v % 2 == 0; });
    assert(n == 5 && h.size() == 5);

    std::hive<int> h2{1, 2, 3, 2, 1};
    auto n2 = std::erase(h2, 2);
    assert(n2 == 2 && h2.size() == 3);
  }

  // splice: self-splice is erroneous-but-harmless (no effects); incompatible
  // block_capacity_limits throws length_error.
  {
    std::hive<int> h;
    h.insert(1);
    h.splice(h);
    assert(h.size() == 1);
  }
  {
    std::hive<int> a(std::hive_limits(4, 4));
    std::hive<int> b(std::hive_limits(64, 64));
    b.insert(1);
    bool threw = false;
    try {
      a.splice(b);
    } catch (const std::length_error&) {
      threw = true;
    }
    assert(threw);
  }

  // allocator-extended copy/move constructors
  {
    std::hive<int> h;
    h.insert(1);
    h.insert(2);
    std::allocator<int> a;
    std::hive<int> h2(h, a);
    assert(h2.size() == 2);
    std::hive<int> h3(std::move(h2), a);
    assert(h3.size() == 2);
  }

  // copy/move assignment
  {
    std::hive<std::string> a;
    a.insert("x");
    a.insert("y");
    std::hive<std::string> b;
    b = a;
    assert(b.size() == 2);
    std::hive<std::string> c;
    c = std::move(b);
    assert(c.size() == 2);
  }

  // clear
  {
    std::hive<int> h{1, 2, 3};
    h.clear();
    assert(h.empty() && h.size() == 0 && h.begin() == h.end());
    h.insert(42);
    assert(h.size() == 1 && *h.begin() == 42);
  }

  return 0;
}
