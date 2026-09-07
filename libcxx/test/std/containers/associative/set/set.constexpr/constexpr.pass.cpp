//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <set>
//
// P3372R3: constexpr containers and adaptors -- set's full member surface
// (constructors, observers, modifiers, comparisons, swap) is constexpr,
// backed by __tree. See map.constexpr/constexpr.pass.cpp for the boundary
// this avoids relative to unordered_map/unordered_set.

#include <set>
#include <utility>

constexpr bool test_set() {
  std::set<int> s;
  for (int i = 0; i < 20; ++i)
    s.insert(i);
  if (s.size() != 20)
    return false;

  if (s.find(15) == s.end())
    return false;
  s.erase(15);
  if (s.find(15) != s.end())
    return false;

  // Duplicate key: no-op, works fine (no goto-based fast path in __tree).
  s.insert(3);
  if (s.size() != 19)
    return false;

  std::set<int> copy(s);
  if (copy.size() != s.size())
    return false;

  std::set<int> moved(std::move(copy));
  if (moved.size() != s.size())
    return false;

  moved.clear();
  return moved.empty();
}
static_assert(test_set());

int main(int, char**) { return 0; }
