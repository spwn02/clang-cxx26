//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <map>

// template<class K>       mapped_type& at(const K& x);       // C++26
// template<class K> const mapped_type& at(const K& x) const; // C++26

// [map.access]: the parameter is const K&, so the heterogeneous at also takes an lvalue when K is
// given explicitly (at<std::string_view>(sv)); a forwarding reference would not bind it.

#include <cassert>
#include <functional>
#include <map>
#include <string>
#include <string_view>

int main(int, char**) {
  std::map<std::string, int, std::less<>> m{{"a", 1}, {"b", 2}};
  const auto& cm = m;
  std::string_view k = "b";

  assert(m.at<std::string_view>(k) == 2);
  assert(cm.at<std::string_view>(k) == 2);
  assert(m.at(k) == 2);
  assert(cm.at(std::string_view("a")) == 1);
  m.at<std::string_view>(k) = 5;
  assert(m.at("b") == 5);

  try {
    (void)m.at<std::string_view>(std::string_view("zzz"));
    assert(false);
  } catch (const std::out_of_range&) {
  }
  return 0;
}
