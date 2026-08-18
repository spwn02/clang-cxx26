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
#include <memory>
#include <string>

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
    // Destructor and move-assign, on a sparsely-filled large-capacity group
    // with a non-trivial element type: both route through
    // __clear_and_deallocate_all(), which delegates to clear() -- this is
    // the same never-constructed-destructor hazard clear() itself had (see
    // modifiers.pass.cpp), just reached without an explicit clear() call.
    {
      std::hive<std::string> h(std::hive_limits(64, 64));
      for (int i = 0; i < 20; ++i)
        h.insert(std::to_string(i));
      // destructor runs here, on a group with a long never-used free run.
    }
    {
      std::hive<std::string> b(std::hive_limits(64, 64));
      for (int i = 0; i < 20; ++i)
        b.insert(std::to_string(i));
      std::hive<std::string> a;
      a.insert("placeholder");
      b = std::move(a); // destroys b's old sparse contents via move-assign
      assert(b.size() == 1 && *b.begin() == "placeholder");
    }
  }

  {
    // sort() must work on a move-only type (Cpp17MoveInsertable /
    // MoveAssignable / Swappable, not CopyConstructible) -- the entire
    // reason it was rewritten to build its scratch buffer via move_iterator
    // instead of copying from the hive's iterators.
    std::hive<std::unique_ptr<int>> h;
    h.emplace(std::make_unique<int>(5));
    h.emplace(std::make_unique<int>(2));
    h.emplace(std::make_unique<int>(4));
    h.emplace(std::make_unique<int>(1));
    h.emplace(std::make_unique<int>(3));
    h.sort([](const std::unique_ptr<int>& a, const std::unique_ptr<int>& b) { return *a < *b; });
    std::vector<int> vals;
    for (auto& p : h)
      vals.push_back(*p);
    assert((vals == std::vector<int>{1, 2, 3, 4, 5}));
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
