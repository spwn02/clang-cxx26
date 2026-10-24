//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// The iterator-conversion exclusions constrain only the non-hint overload.

#include <cassert>
#include <map>

struct IteratorConvertibleKey;

struct TransparentLess {
  using is_transparent = void;

  template <class T, class U>
  bool operator()(const T& lhs, const U& rhs) const {
    return static_cast<int>(lhs) < static_cast<int>(rhs);
  }
};

using Map = std::map<int, int, TransparentLess>;

struct IteratorConvertibleKey {
  int value;

  explicit operator int() const { return value; }
  operator Map::iterator() const;
};

static_assert(requires(Map& map, Map::const_iterator hint, IteratorConvertibleKey key) { map.try_emplace(hint, key); });

int main(int, char**) {
  Map map;
  auto it = map.try_emplace(map.end(), IteratorConvertibleKey{3}, 4);
  assert(it->first == 3 && it->second == 4);
  return 0;
}
