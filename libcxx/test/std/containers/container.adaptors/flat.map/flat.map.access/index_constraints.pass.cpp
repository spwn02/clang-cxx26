//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <flat_map>
// operator[] has no constraints for key_type. The heterogeneous overload
// is constrained only by Compare::is_transparent.

#include <flat_map>

struct NoDefault {
  NoDefault() = delete;
  explicit NoDefault(int);
};

struct OtherKey {};

struct TransparentCompare {
  using is_transparent = void;
  bool operator()(int, int) const;
  bool operator()(int, OtherKey) const;
  bool operator()(OtherKey, int) const;
};

template <class Map>
concept HasKeySubscript = requires(Map& map, const typename Map::key_type& key) { map[key]; };

template <class Map>
concept HasOtherSubscript = requires(Map& map, OtherKey key) { map[key]; };

static_assert(HasKeySubscript<std::flat_map<int, NoDefault>>);
static_assert(HasOtherSubscript<std::flat_map<int, int, TransparentCompare>>);
static_assert(!HasOtherSubscript<std::flat_map<int, int>>);

int main(int, char**) {
  std::flat_map<int, int> map;
  map[1] = 2;
  return map[1] == 2 ? 0 : 1;
}
