//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <map>
//
// P3372R3: constexpr containers and adaptors -- map's full member surface
// (constructors, observers, modifiers, comparisons, swap) is constexpr,
// backed by __tree, which already used the union-based deferred-init node
// pattern list/forward_list rely on. Unlike unordered_map, there is no
// bucket array and std::less has none of std::hash's type-punning, so this
// avoids most boundaries documented for unordered_map/unordered_set in
// docs/CXX26_GAPS.md -- including duplicate-key insertion, which works fine
// here (no goto-based fast path in __tree's lookup). It does hit the same
// const_cast-based in-place key reuse during same-size copy-assignment that
// unordered_map does (__tree:1469, same shape as __hash_table:1103) -- not
// exercised here, see docs/CXX26_GAPS.md.

#include <map>
#include <utility>

constexpr bool test_map() {
  std::map<int, int> m;
  if (!m.empty() || m.size() != 0)
    return false;
  for (int i = 0; i < 20; ++i)
    m.emplace(i, i * i);
  if (m.size() != 20)
    return false;

  auto it = m.find(10);
  if (it == m.end() || it->second != 100)
    return false;
  if (m.at(10) != 100)
    return false;

  m[10] = -1;
  if (m.at(10) != -1)
    return false;

  m.erase(5);
  if (m.find(5) != m.end())
    return false;

  // Duplicate key: no-op, unlike unordered_map's goto-based fast path this
  // works fine in constant evaluation.
  m.emplace(10, 999);
  if (m.at(10) != -1)
    return false;

  std::map<int, int> copy(m);
  if (copy.size() != m.size())
    return false;

  std::map<int, int> moved(std::move(copy));
  if (moved.size() != m.size())
    return false;

  moved.clear();
  return moved.empty();
}
static_assert(test_map());

int main(int, char**) { return 0; }
