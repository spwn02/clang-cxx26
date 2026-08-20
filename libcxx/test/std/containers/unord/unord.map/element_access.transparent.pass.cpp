//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <unordered_map>

// class unordered_map

// template<class K> mapped_type& operator[](K&& k);          // C++26
// template<class K>       mapped_type& at(const K& x);       // C++26
// template<class K> const mapped_type& at(const K& x) const; // C++26

#include <unordered_map>
#include <cassert>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "test_macros.h"

struct transparent_hash {
  using is_transparent = void;
  size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

int main(int, char**) {
  using M = std::unordered_map<std::string, int, transparent_hash, std::equal_to<>>;
  M m{{"hello", 1}, {"world", 3}};

  {
    int& v = m[std::string_view("hello")];
    assert(v == 1);
    v = 42;
    assert(m.at(std::string("hello")) == 42);

    int& v2 = m[std::string_view("baz")];
    assert(v2 == 0);
    assert(m.size() == 3);
  }

  {
    assert(m.at(std::string_view("hello")) == 42);
    const M& cm = m;
    ASSERT_SAME_TYPE(decltype(cm.at(std::string_view("world"))), const int&);
    assert(cm.at(std::string_view("world")) == 3);

    bool threw = false;
    try {
      (void)m.at(std::string_view("nope"));
    } catch (const std::out_of_range&) {
      threw = true;
    }
    assert(threw);
  }

  return 0;
}
