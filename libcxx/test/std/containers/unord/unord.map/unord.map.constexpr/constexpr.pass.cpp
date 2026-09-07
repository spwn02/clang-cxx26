//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <unordered_map>
//
// P3372R3: constexpr containers and adaptors -- unordered_map's member
// surface is constexpr for integral/enum/nullptr_t keys, as long as bucket
// growth stays on a power-of-two trajectory and no operation looks up a key
// that already exists. See docs/CXX26_GAPS.md for the full set of boundaries
// this hits (std::hash's union-based scalar hashing, __next_prime being an
// ABI-exported non-inline function, and a goto-based fast path in the
// duplicate-key lookup) that are not fixed by this change.

#include <unordered_map>
#include <utility>

constexpr bool test_unordered_map() {
  std::unordered_map<int, int> m;
  m.reserve(16); // power-of-two growth avoids __next_prime, see docs/CXX26_GAPS.md
  if (!m.empty() || m.size() != 0)
    return false;
  for (int i = 0; i < 10; ++i)
    m.emplace(i, i * i);
  if (m.size() != 10)
    return false;

  auto it = m.find(5);
  if (it == m.end() || it->second != 25)
    return false;
  if (m.at(5) != 25)
    return false;

  m[10] = 100; // a new key -- avoids the duplicate-key goto path
  if (m.at(10) != 100)
    return false;

  m.erase(3);
  if (m.find(3) != m.end())
    return false;

  std::unordered_map<int, int> copy(m);
  if (copy.size() != m.size())
    return false;

  std::unordered_map<int, int> moved(std::move(copy));
  if (moved.size() != m.size())
    return false;

  moved.clear();
  return moved.empty();
}
static_assert(test_unordered_map());

// Documented boundary: emplace/insert of a key that already exists hits a
// goto-based fast path (skip constructing a duplicate node) that this
// compiler's constant evaluator does not support at all -- confirmed via a
// standalone goto-in-constexpr test unrelated to unordered_map.
static_assert(!__builtin_constant_p([] {
  std::unordered_map<int, int> m;
  m.reserve(4);
  m.emplace(1, 1);
  m.emplace(1, 2); // duplicate key -> hits goto
  return m.size();
}()));

int main(int, char**) { return 0; }
