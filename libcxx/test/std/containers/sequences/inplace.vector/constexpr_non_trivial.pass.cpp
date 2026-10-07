//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// P3074R7: inplace_vector of non-trivial types in constant expressions (__cpp_lib_constexpr_inplace_vector).

#include <cassert>
#include <inplace_vector>
#include <string>
#include <utility>

#ifndef __cpp_lib_constexpr_inplace_vector
#  error __cpp_lib_constexpr_inplace_vector should be defined
#endif
#if __cpp_lib_constexpr_inplace_vector != 202502L
#  error __cpp_lib_constexpr_inplace_vector should have the value 202502L
#endif

#include "test_macros.h"

struct Tracked {
  int* live;
  constexpr Tracked(int* l) : live(l) { ++*live; }
  constexpr Tracked(const Tracked& o) : live(o.live) { ++*live; }
  constexpr Tracked(Tracked&& o) : live(o.live) { ++*live; }
  constexpr Tracked& operator=(const Tracked&) = default;
  constexpr ~Tracked() { --*live; }
};

constexpr bool test() {
  {
    std::inplace_vector<std::string, 4> v;
    v.push_back("a string that is long enough to need an allocation 0123456789");
    v.emplace_back("b");
    v.push_back(std::string(30, 'c'));
    auto w = v; // copy
    auto u = std::move(w); // move
    v.pop_back();
    v.insert(v.begin(), "front");
    v.erase(v.begin() + 1);
    if (!(v.size() == 2 && v[0] == "front" && v[1] == "b" && u.size() == 3 && u[0].size() > 20 && u[2].size() == 30))
      return false;
    v.assign(2, std::string("same"));
    v.insert(v.begin() + 1, 1, std::string("x"));
    if (!(v.size() == 3 && v[0] == "same" && v[1] == "x" && v[2] == "same"))
      return false;
    std::inplace_vector<std::string, 4> a = {"p", "q"};
    a.swap(v);
    if (!(a.size() == 3 && v.size() == 2 && v[1] == "q"))
      return false;
  }

  // lifetimes: every element is constructed and destroyed exactly once
  int live = 0;
  {
    std::inplace_vector<Tracked, 3> t;
    t.emplace_back(&live);
    t.emplace_back(&live);
    auto c = t;
    if (live != 4)
      return false;
    t.clear();
    if (live != 2)
      return false;
    t.push_back(c[0]);
    t.insert(t.begin(), c[1]);
    if (live != 4 || t.size() != 2)
      return false;
  }
  return live == 0;
}

int main(int, char**) {
  assert(test());
  static_assert(test());
  return 0;
}
