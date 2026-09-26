//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <flat_map>, <flat_set>

// LWG 3816: the flat containers require their underlying containers to be sequence containers, which
// std::array is not (it cannot insert or erase).

#include <array>
#include <flat_map>
#include <flat_set>

// One diagnostic per container adaptor (the first failing static_assert in each).
// expected-error@*:* 4 {{std::array is not a sequence container}}
// expected-note@*:* 4 {{in instantiation of template class}}
void test() {
  using Array = std::array<int, 3>;
  std::flat_map<int, int, std::less<int>, Array, Array> m;
  std::flat_multimap<int, int, std::less<int>, Array, Array> mm;
  std::flat_set<int, std::less<int>, Array> s;
  std::flat_multiset<int, std::less<int>, Array> ms;
  (void)m;
  (void)mm;
  (void)s;
  (void)ms;
}
