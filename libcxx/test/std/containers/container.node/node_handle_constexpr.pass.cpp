//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// P3372R3: constexpr containers and adaptors -- __basic_node_handle's
// member surface is constexpr except key(), excluded per CWG2514 (matching
// upstream P3372R3's own carve-out). Depends on both __tree and
// __hash_table's node destructors being constexpr, so this only became
// possible once map/set and unordered_map/unordered_set were both done.

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

constexpr bool test_map_node() {
  std::map<int, int> m;
  m.emplace(1, 100);
  m.emplace(2, 200);
  auto nh = m.extract(1);
  if (nh.empty() || nh.mapped() != 100)
    return false;
  nh.mapped() = 999;
  auto res = m.insert(std::move(nh));
  if (!res.inserted || m.find(1) == m.end() || m.at(1) != 999)
    return false;
  return m.size() == 2;
}
static_assert(test_map_node());

constexpr bool test_set_node() {
  std::set<int> s;
  s.emplace(1);
  s.emplace(2);
  auto nh = s.extract(1);
  if (nh.empty() || nh.value() != 1)
    return false;
  nh.value() = 3;
  auto res = s.insert(std::move(nh));
  if (!res.inserted || s.find(3) == s.end())
    return false;
  return s.size() == 2;
}
static_assert(test_set_node());

constexpr bool test_unordered_map_node() {
  std::unordered_map<int, int> m;
  m.reserve(4); // power-of-two growth avoids __next_prime, see docs/CXX26_GAPS.md
  m.emplace(1, 100);
  m.emplace(2, 200);
  auto nh = m.extract(1);
  if (nh.empty() || nh.mapped() != 100)
    return false;
  nh.mapped() = 999;
  auto res = m.insert(std::move(nh));
  if (!res.inserted || m.find(1) == m.end() || m.at(1) != 999)
    return false;
  return m.size() == 2;
}
static_assert(test_unordered_map_node());

constexpr bool test_unordered_set_node() {
  std::unordered_set<int> s;
  s.reserve(4);
  s.emplace(1);
  s.emplace(2);
  auto nh = s.extract(1);
  if (nh.empty() || nh.value() != 1)
    return false;
  nh.value() = 3;
  auto res = s.insert(std::move(nh));
  if (!res.inserted || s.find(3) == s.end())
    return false;
  return s.size() == 2;
}
static_assert(test_unordered_set_node());

// Documented boundary: key() is excluded from constexpr per CWG2514.
static_assert(!__builtin_constant_p([] {
  std::map<int, int> m;
  m.emplace(1, 1);
  auto nh = m.extract(1);
  return nh.key();
}()));

int main(int, char**) { return 0; }
