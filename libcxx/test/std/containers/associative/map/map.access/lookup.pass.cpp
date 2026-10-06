//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <map>

// P3091R6: optional<mapped_type&> lookup(const key_type&), lookup(const K&) for transparent comparators.

#include <map>
#include <cassert>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "test_macros.h"

#ifndef __cpp_lib_map_lookup
#  error __cpp_lib_map_lookup should be defined
#endif
#if __cpp_lib_map_lookup != 202606L
#  error __cpp_lib_map_lookup should have the value 202606L
#endif

struct NotAKey {};
template <class M, class K>
concept has_lookup = requires(M& m, const K& k) { m.lookup(k); };

struct Transparent {
  using is_transparent = void;
  bool operator()(int a, int b) const { return a < b; }
  bool operator()(int a, long b) const { return a < b; }
  bool operator()(long a, int b) const { return a < b; }
};
struct TransparentHash {
  using is_transparent = void;
  std::size_t operator()(long x) const { return std::hash<long>()(x); }
};
struct TransparentEqual {
  using is_transparent = void;
  bool operator()(long a, long b) const { return a == b; }
};

bool test() {
  {
    std::map<int, int> m = {{1, 10}, {2, 20}};
    static_assert(std::is_same_v<decltype(m.lookup(1)), std::optional<int&>>);
    static_assert(std::is_same_v<decltype(std::as_const(m).lookup(1)), std::optional<const int&>>);
    std::optional<int&> r = m.lookup(2);
    assert(r.has_value() && &*r == &m.find(2)->second);
    *r = 21;
    assert(m.at(2) == 21);
    assert(!m.lookup(3).has_value());
    const auto& cm = m;
    assert(cm.lookup(1).value() == 10);
    assert(!cm.lookup(0));
    // a comparator that is not transparent: the template overloads do not participate
    static_assert(!has_lookup<decltype(m), NotAKey>);
  }
  {
    std::map<int, int, Transparent> m = {{1, 10}, {2, 20}};
    static_assert(has_lookup<decltype(m), long>);
    assert(m.lookup(1L).value() == 10);
    assert(!m.lookup(5L));
    const auto& cm = m;
    static_assert(std::is_same_v<decltype(cm.lookup(1L)), std::optional<const int&>>);
    assert(cm.lookup(2L).value() == 20);
  }
  return true;
}

int main(int, char**) {
  test();
  return 0;
}
