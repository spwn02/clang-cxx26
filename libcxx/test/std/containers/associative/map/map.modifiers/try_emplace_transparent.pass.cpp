//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <map>

// class map

// template <class K, class... Args>
//     pair<iterator, bool> try_emplace(K&& k, Args&&... args);                  // C++26
// template <class K, class... Args>
//     iterator try_emplace(const_iterator hint, K&& k, Args&&... args);         // C++26

#include <cassert>
#include <map>
#include <string>
#include <string_view>
#include <utility>

#include "test_macros.h"

struct transparent_less {
  using is_transparent = void;
  template <class T, class U>
  bool operator()(const T& a, const U& b) const {
    return a < b;
  }
};

int main(int, char**) {
  // non-hint overload
  {
    std::map<std::string, int, transparent_less> m;

    auto [it1, ins1] = m.try_emplace(std::string_view("hello"), 1);
    assert(ins1);
    assert(it1->first == "hello" && it1->second == 1);

    // Existing key: value is untouched, existing element's iterator returned.
    auto [it2, ins2] = m.try_emplace(std::string_view("hello"), 2);
    assert(!ins2);
    assert(it2 == it1);
    assert(it2->second == 1);

    ASSERT_SAME_TYPE(decltype(m.try_emplace(std::string_view("x"), 0)), std::pair<decltype(m)::iterator, bool>);
  }

  // hint overload
  {
    std::map<std::string, int, transparent_less> m;
    auto it = m.try_emplace(m.end(), std::string_view("world"), 3);
    assert(it->first == "world" && it->second == 3);

    auto it2 = m.try_emplace(m.begin(), std::string_view("world"), 4);
    assert(it2->second == 3); // unchanged: key already present

    ASSERT_SAME_TYPE(decltype(m.try_emplace(m.begin(), std::string_view("x"), 0)), decltype(m)::iterator);
  }

  // Regression test: on a map with a transparent comparator, calls using the *homogeneous*
  // key_type must still resolve unambiguously (the heterogeneous templates additionally
  // compete for overload resolution once the comparator is transparent).
  {
    std::map<std::string, int, transparent_less> m;
    std::string k = "a";

    // lvalue key_type: the heterogeneous template is now viable too (K&& binds std::string&,
    // which is at least as good a match as the const key_type& overload), but must still behave
    // correctly.
    auto [it1, ins1] = m.try_emplace(k, 1);
    assert(ins1 && it1->second == 1);

    // rvalue key_type: must still pick a move-capable path (the non-template key_type&&
    // overload wins ties against a template specialization per [over.match.best]).
    auto [it2, ins2] = m.try_emplace(std::string("b"), 2);
    assert(ins2 && it2->second == 2);

    // The critical disambiguation case the exclusion constraint on try_emplace(K&&, Args&&...)
    // exists for: a 2-argument call (hint, key) with an empty Args pack. Without
    // `!is_convertible_v<K&&, const_iterator>`, this would be ambiguous between the hint
    // overload and the heterogeneous non-hint overload (K = const_iterator, Args = {string}).
    auto it3 = m.try_emplace(m.begin(), k);
    assert(it3->second == 1); // key "a" already present; mapped_type left as-is
    assert(m.size() == 2);
  }

  return 0;
}
