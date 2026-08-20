//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <unordered_set>

// class unordered_set

// template <class K> pair<iterator, bool> insert(K&& x);                  // C++26
// template <class K> iterator insert(const_iterator hint, K&& x);         // C++26

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <unordered_set>
#include <cassert>
#include <functional>
#include <string>
#include <string_view>

#include "test_macros.h"

struct transparent_hash {
  using is_transparent = void;
  size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

int main(int, char**) {
  using S = std::unordered_set<std::string, transparent_hash, std::equal_to<>>;

  // non-hint overload
  {
    S s;
    auto [it1, ins1] = s.insert(std::string_view("hello"));
    assert(ins1);
    assert(*it1 == "hello");

    auto [it2, ins2] = s.insert(std::string_view("hello"));
    assert(!ins2);
    assert(it2 == it1);
    assert(s.size() == 1);
  }

  // hint overload
  {
    S s;
    auto it = s.insert(s.end(), std::string_view("world"));
    assert(*it == "world");

    auto it2 = s.insert(s.begin(), std::string_view("world"));
    assert(*it2 == "world");
    assert(s.size() == 1);
  }

  // Regression test: homogeneous-key calls (including the 2-iterator range-insert overload,
  // which the hint overload's exclusion constraint exists to disambiguate from) must still
  // resolve unambiguously on a transparent container.
  {
    S s;
    std::string k = "a";

    auto [it1, ins1] = s.insert(k); // lvalue key_type
    assert(ins1 && *it1 == "a");

    auto [it2, ins2] = s.insert(std::string("b")); // rvalue key_type
    assert(ins2 && *it2 == "b");

    auto it3 = s.insert(s.begin(), k); // hint overload, homogeneous key
    assert(*it3 == "a");

    std::string arr[] = {"c", "d"};
    s.insert(arr, arr + 2); // 2-iterator range insert: must not collide with insert(hint, K&&)
    assert(s.size() == 4);
  }

  return 0;
}
