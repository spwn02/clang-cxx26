//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// from_range_t construction, assign_range, append_range, insert_range; the
// N == 0 specialization; and the LWG 4122 fix making operator<=> drop out
// of overload resolution (instead of hard-erroring) for a T with no viable
// three-way comparison.

#include <inplace_vector>
#include <array>
#include <cassert>
#include <compare>
#include <concepts>
#include <stdexcept>
#include <vector>

#include "test_macros.h"

struct NoSpaceship {
  int x;
  bool operator==(const NoSpaceship&) const = default;
};

int main(int, char**) {
  {
    std::inplace_vector<int, 4> v(std::from_range, std::array<int, 3>{1, 2, 3});
    assert(v.size() == 3 && v[0] == 1 && v[2] == 3);
  }

  {
    std::inplace_vector<int, 4> v{1, 2};
    v.append_range(std::array<int, 2>{3, 4});
    assert(v.size() == 4 && v[3] == 4);

    bool threw = false;
    try {
      v.append_range(std::array<int, 1>{5}); // capacity overflow -> bad_alloc
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    assert(threw);
    assert(v.size() == 4); // no effect on overflow
  }

  {
    std::inplace_vector<int, 4> v{9, 9, 9};
    v.assign_range(std::array<int, 2>{7, 8});
    assert(v.size() == 2 && v[0] == 7 && v[1] == 8);
  }

  {
    std::inplace_vector<int, 4> v{1, 4};
    v.insert_range(v.begin() + 1, std::array<int, 2>{2, 3});
    assert(v.size() == 4 && v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4);
  }

  // N == 0 specialization.
  {
    std::inplace_vector<int, 0> v;
    assert(v.empty() && v.size() == 0 && v.capacity() == 0 && v.max_size() == 0);
    assert(v.begin() == v.end());
    assert(v.data() == nullptr);
    assert(!v.try_push_back(1).has_value());

    bool threw = false;
    try {
      v.push_back(1);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    assert(threw);

    // at() is specified to throw, not a precondition violation, so it's
    // safe to actually call for every index.
    threw = false;
    try {
      (void)v.at(0);
    } catch (const std::out_of_range&) {
      threw = true;
    }
    assert(threw);

    // operator[]/front()/back()/unchecked_push_back()/unchecked_emplace_back()
    // all have unsatisfiable preconditions for N == 0 (no valid index or
    // dereferenceable position ever exists) and calling them is UB -- check
    // only that they exist with the right shape, without invoking them.
    static_assert(requires(std::inplace_vector<int, 0>& iv) {
      { iv[0] } -> std::same_as<int&>;
      { iv.front() } -> std::same_as<int&>;
      { iv.back() } -> std::same_as<int&>;
      { iv.unchecked_push_back(1) } -> std::same_as<int&>;
      { iv.unchecked_emplace_back(1) } -> std::same_as<int&>;
    });

    // Every InputIt/range/count/initializer_list assign is well-defined as
    // long as it contributes zero elements.
    v.assign(0, 1);
    int __empty_src[1]{};
    v.assign(__empty_src, __empty_src);
    v.assign({});
    v.assign_range(std::array<int, 0>{});
    v = {};
    assert(v.empty());

    // erase() of the (only ever valid) empty range at end() is well-defined.
    assert(v.erase(v.begin(), v.end()) == v.end());

    std::inplace_vector<int, 0> v2 = v; // trivial copy must still work
    assert(v == v2);
  }

  // LWG 4122: operator<=> must not hard-error for a T with no viable
  // three-way comparison -- it should simply not participate.
  {
    static_assert(std::equality_comparable<std::inplace_vector<NoSpaceship, 2>>);
    static_assert(!std::three_way_comparable<std::inplace_vector<NoSpaceship, 2>>);

    std::inplace_vector<NoSpaceship, 2> a{NoSpaceship{1}}, b{NoSpaceship{1}};
    assert(a == b);
  }

  return 0;
}
