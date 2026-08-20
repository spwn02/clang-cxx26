//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <unordered_set>

// class unordered_multiset

// template<class K> size_type bucket(const K& k) const;      // C++26

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
  using S = std::unordered_multiset<std::string, transparent_hash, std::equal_to<>>;
  S s{"hello", "hello"};
  s.rehash(8);

  ASSERT_SAME_TYPE(decltype(s.bucket(std::string_view("hello"))), S::size_type);
  assert(s.bucket(std::string_view("hello")) == s.bucket(std::string("hello")));

  return 0;
}
