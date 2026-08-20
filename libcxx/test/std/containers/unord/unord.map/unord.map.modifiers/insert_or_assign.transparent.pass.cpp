//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <unordered_map>

// class unordered_map

// template <class K, class M>
//  pair<iterator, bool> insert_or_assign(K&& k, M&& obj);                       // C++26
// template <class K, class M>
//  iterator insert_or_assign(const_iterator hint, K&& k, M&& obj);              // C++26

#include <unordered_map>
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
  using M = std::unordered_map<std::string, int, transparent_hash, std::equal_to<>>;

  // non-hint overload
  {
    M m;
    auto [it1, ins1] = m.insert_or_assign(std::string_view("hello"), 1);
    assert(ins1);
    assert(it1->second == 1);

    auto [it2, ins2] = m.insert_or_assign(std::string_view("hello"), 2);
    assert(!ins2);
    assert(it2 == it1);
    assert(it2->second == 2);
  }

  // hint overload
  {
    M m;
    auto it = m.insert_or_assign(m.end(), std::string_view("world"), 3);
    assert(it->second == 3);

    auto it2 = m.insert_or_assign(m.begin(), std::string_view("world"), 4);
    assert(it2->second == 4);
  }

  // Regression test: homogeneous key_type calls must still resolve correctly once the
  // container's hasher/key_equal are transparent.
  {
    M m;
    std::string k = "a";

    auto [it1, ins1] = m.insert_or_assign(k, 1); // lvalue key_type
    assert(ins1 && it1->second == 1);

    auto [it2, ins2] = m.insert_or_assign(std::string("b"), 2); // rvalue key_type
    assert(ins2 && it2->second == 2);

    auto it3 = m.insert_or_assign(m.begin(), k, 9); // hint overload, homogeneous key
    assert(it3->second == 9);
    assert(m.size() == 2);
  }

  return 0;
}
