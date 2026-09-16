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
// growth can use either bucket trajectory. Scalar std::hash's union-based
// type-punning remains a separate constant-evaluation boundary.

#include <unordered_map>
#include <utility>

// A hash that collapses every key into one of two buckets, forcing a real
// multi-node collision chain -- unlike the default hash used below, where
// small distinct keys mostly land in distinct buckets and the duplicate-key
// lookup never has to walk past the first node in the chain.
struct colliding_hash {
  constexpr std::size_t operator()(int key) const { return static_cast<std::size_t>(key) % 2; }
};

constexpr bool test_unordered_map_collision_chain() {
  std::unordered_map<int, int, colliding_hash> m;
  m.reserve(2);
  for (int i = 0; i < 8; i += 2) // 0, 2, 4, 6 -- all hash to bucket 0
    m.emplace(i, i);
  if (m.size() != 4)
    return false;
  // Duplicate-key emplace of a key that is NOT the first node in its bucket's
  // chain: must still be detected as a duplicate, not silently inserted
  // again (the goto-to-flag rewrite must walk the whole chain, not just the
  // first node before breaking out of the loop).
  if (m.emplace(6, -1).second || m.at(6) != 6)
    return false;
  if (m.emplace(0, -1).second || m.at(0) != 0)
    return false;
  if (m.size() != 4)
    return false;
  return true;
}
static_assert(test_unordered_map_collision_chain());

constexpr bool test_unordered_map() {
  std::unordered_map<int, int> m;
  m.reserve(17); // non-power-of-two growth exercises constexpr __next_prime
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
  if (m.emplace(10, 999).second || m.at(10) != 100)
    return false;

  m.erase(3);
  if (m.find(3) != m.end())
    return false;

  std::unordered_map<int, int> copy(m);
  if (copy.size() != m.size())
    return false;
  copy = m; // same-size assignment reconstructs const-key values in place
  if (copy.at(10) != 100)
    return false;

  std::unordered_map<int, int> moved(std::move(copy));
  if (moved.size() != m.size())
    return false;

  moved.clear();
  return moved.empty();
}
static_assert(test_unordered_map());

int main(int, char**) { return 0; }
