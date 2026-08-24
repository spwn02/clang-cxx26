//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// P2248R8 (Enabling list-initialization for algorithms) + P3217R0 (its
// find_last addendum): the non-range algorithms (including their
// ExecutionPolicy overloads), the container erase() free functions, and
// every ranges:: algorithm the two papers touch (fold_left/
// fold_left_with_iter, find/count/search_n/replace/replace_if/
// replace_copy/replace_copy_if/fill/fill_n/remove/remove_copy/
// lower_bound/upper_bound/equal_range/binary_search/contains/find_last).
// ranges::fold_right is unimplementable here (a separate, pre-existing
// gap from P2322R6, not this paper) and ranges::find_last_if/
// find_last_if_not take a predicate rather than a value, so neither
// needed the treatment. See docs/CXX26_GAPS.md Tier 6.
//
// This test exercises the actual capability the paper is for: calling
// each affected algorithm/erase() with a braced-init-list value, which
// requires the value-type template parameter to have a default (a
// braced-init-list is a non-deduced context against a bare template
// parameter).

#include <algorithm>
#include <cassert>
#include <concepts>
#include <deque>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <ranges>
#include <string>
#include <vector>

#if _LIBCPP_HAS_EXPERIMENTAL_PSTL
#  include <execution>
#endif

#include "test_macros.h"

struct Point {
  int x;
  int y;
  friend bool operator==(const Point&, const Point&) = default;
};

struct LessByX {
  constexpr bool operator()(const Point& lhs, const Point& rhs) const { return lhs.x < rhs.x; }
};

struct AddPoints {
  constexpr Point operator()(Point acc, Point p) const { return Point{acc.x + p.x, acc.y + p.y}; }
};

struct ToDouble {
  constexpr double operator()(int i) const { return i; }
};

// [projected], projected_value_t
static_assert(std::same_as<std::projected_value_t<int*, std::identity>, int>);
static_assert(std::same_as<std::projected_value_t<int*, ToDouble>, double>);
static_assert(std::same_as<std::projected_value_t<Point*, decltype(&Point::x)>, int>);

constexpr bool test() {
  // fill / fill_n
  {
    std::vector<Point> v(3);
    std::fill(v.begin(), v.end(), {1, 2});
    assert((v == std::vector<Point>{{1, 2}, {1, 2}, {1, 2}}));

    std::vector<Point> v2(3);
    std::fill_n(v2.begin(), 3, {3, 4});
    assert((v2 == std::vector<Point>{{3, 4}, {3, 4}, {3, 4}}));
  }

  // find / count
  {
    std::vector<Point> v{{1, 2}, {3, 4}, {5, 6}};
    assert(std::find(v.begin(), v.end(), {3, 4}) == v.begin() + 1);
    assert(std::count(v.begin(), v.end(), {3, 4}) == 1);
  }

  // search_n (both the predicate and non-predicate overloads)
  {
    std::vector<Point> v{{1, 1}, {2, 2}, {2, 2}, {3, 3}};
    assert(std::search_n(v.begin(), v.end(), 2, {2, 2}) == v.begin() + 1);
    assert(std::search_n(v.begin(), v.end(), 2, {2, 2}, [](const Point& a, const Point& b) {
             return a.x == b.x;
           }) == v.begin() + 1);
  }

  // replace / replace_if / replace_copy_if (replace_copy is deliberately excluded by the paper)
  {
    std::vector<Point> v{{1, 2}, {3, 4}};
    std::replace(v.begin(), v.end(), {1, 2}, {9, 9});
    assert((v == std::vector<Point>{{9, 9}, {3, 4}}));

    std::vector<Point> v2{{1, 2}, {3, 4}};
    std::replace_if(
        v2.begin(), v2.end(), [](const Point& p) { return p.x == 1; }, {7, 7});
    assert((v2 == std::vector<Point>{{7, 7}, {3, 4}}));

    std::vector<Point> src{{1, 2}, {3, 4}};
    std::vector<Point> dst(2);
    std::replace_copy_if(
        src.begin(), src.end(), dst.begin(), [](const Point& p) { return p.x == 1; }, {8, 8});
    assert((dst == std::vector<Point>{{8, 8}, {3, 4}}));
  }

  // remove / remove_copy
  {
    std::vector<Point> v{{1, 2}, {3, 4}, {1, 2}};
    assert(std::remove(v.begin(), v.end(), {1, 2}) == v.begin() + 1);

    std::vector<Point> src{{1, 2}, {3, 4}, {1, 2}};
    std::vector<Point> out(3);
    assert(std::remove_copy(src.begin(), src.end(), out.begin(), {1, 2}) == out.begin() + 1);
  }

  // lower_bound / upper_bound / equal_range / binary_search (with and without an explicit Compare)
  {
    std::vector<Point> v{{1, 0}, {2, 0}, {2, 0}, {3, 0}};
    assert(std::lower_bound(v.begin(), v.end(), {2, 0}, LessByX()) == v.begin() + 1);
    assert(std::upper_bound(v.begin(), v.end(), {2, 0}, LessByX()) == v.begin() + 3);
    auto er = std::equal_range(v.begin(), v.end(), {2, 0}, LessByX());
    assert(er.first == v.begin() + 1 && er.second == v.begin() + 3);
    assert(std::binary_search(v.begin(), v.end(), {2, 0}, LessByX()));

    std::vector<int> nums{1, 2, 2, 3};
    assert(*std::lower_bound(nums.begin(), nums.end(), 2) == 2);
    assert(*std::upper_bound(nums.begin(), nums.end(), 2) == 3);
    assert(std::binary_search(nums.begin(), nums.end(), 2));
  }

  // ranges::fold_left / ranges::fold_left_with_iter (iterator-pair and range overloads).
  // ranges::fold_right isn't implemented in this fork at all (a separate, pre-existing gap
  // unrelated to this paper), so it's not exercised here.
  {
    std::vector<Point> v{{1, 2}, {3, 4}};

    auto sum = std::ranges::fold_left(v.begin(), v.end(), {0, 0}, AddPoints());
    assert((sum == Point{4, 6}));

    auto sum2 = std::ranges::fold_left(v, {0, 0}, AddPoints());
    assert((sum2 == Point{4, 6}));

    auto result = std::ranges::fold_left_with_iter(v.begin(), v.end(), {0, 0}, AddPoints());
    assert((result.value == Point{4, 6}) && result.in == v.end());
  }

  // ranges:: find / count / search_n / replace / replace_if / replace_copy / replace_copy_if /
  // fill / fill_n / remove / remove_copy / lower_bound / upper_bound / equal_range /
  // binary_search / contains / find_last -- each exercised via both the iterator-pair and the
  // range overload, with a braced-init-list value (the capability P2248R8's ranges:: slice adds).
  {
    std::vector<Point> v{{1, 2}, {3, 4}, {5, 6}};
    assert(std::ranges::find(v.begin(), v.end(), {3, 4}) == v.begin() + 1);
    assert(std::ranges::find(v, {3, 4}) == v.begin() + 1);
    assert(std::ranges::count(v.begin(), v.end(), {3, 4}) == 1);
    assert(std::ranges::count(v, {3, 4}) == 1);
    assert(std::ranges::contains(v.begin(), v.end(), {3, 4}));
    assert(std::ranges::contains(v, {3, 4}));
    assert(std::ranges::find_last(v.begin(), v.end(), {3, 4}).begin() == v.begin() + 1);
    assert(std::ranges::find_last(v, {3, 4}).begin() == v.begin() + 1);
  }

  {
    std::vector<Point> v{{1, 1}, {2, 2}, {2, 2}, {3, 3}};
    assert(std::ranges::search_n(v.begin(), v.end(), 2, {2, 2}).begin() == v.begin() + 1);
    assert(std::ranges::search_n(v, 2, {2, 2}).begin() == v.begin() + 1);
    assert(std::ranges::search_n(v.begin(), v.end(), 2, {2, 2}, [](const Point& a, const Point& b) {
                                    return a.x == b.x;
                                  }).begin() == v.begin() + 1);
  }

  {
    std::vector<Point> v{{1, 2}, {3, 4}};
    std::ranges::replace(v.begin(), v.end(), {1, 2}, {9, 9});
    assert((v == std::vector<Point>{{9, 9}, {3, 4}}));

    std::vector<Point> v2{{1, 2}, {3, 4}};
    std::ranges::replace(v2, {1, 2}, {9, 9});
    assert((v2 == std::vector<Point>{{9, 9}, {3, 4}}));

    std::vector<Point> v3{{1, 2}, {3, 4}};
    std::ranges::replace_if(
        v3.begin(), v3.end(), [](const Point& p) { return p.x == 1; }, {7, 7});
    assert((v3 == std::vector<Point>{{7, 7}, {3, 4}}));

    std::vector<Point> v4{{1, 2}, {3, 4}};
    std::ranges::replace_if(v4, [](const Point& p) { return p.x == 1; }, {7, 7});
    assert((v4 == std::vector<Point>{{7, 7}, {3, 4}}));

    std::vector<Point> src{{1, 2}, {3, 4}};
    std::vector<Point> dst(2);
    std::ranges::replace_copy(src.begin(), src.end(), dst.begin(), {1, 2}, {8, 8});
    assert((dst == std::vector<Point>{{8, 8}, {3, 4}}));

    std::vector<Point> dst2(2);
    std::ranges::replace_copy(src, dst2.begin(), {1, 2}, {8, 8});
    assert((dst2 == std::vector<Point>{{8, 8}, {3, 4}}));

    std::vector<Point> dst3(2);
    std::ranges::replace_copy_if(
        src.begin(), src.end(), dst3.begin(), [](const Point& p) { return p.x == 1; }, {8, 8});
    assert((dst3 == std::vector<Point>{{8, 8}, {3, 4}}));

    std::vector<Point> dst4(2);
    std::ranges::replace_copy_if(src, dst4.begin(), [](const Point& p) { return p.x == 1; }, {8, 8});
    assert((dst4 == std::vector<Point>{{8, 8}, {3, 4}}));
  }

  {
    std::vector<Point> v(3);
    std::ranges::fill(v.begin(), v.end(), {1, 2});
    assert((v == std::vector<Point>{{1, 2}, {1, 2}, {1, 2}}));

    std::vector<Point> v2(3);
    std::ranges::fill(v2, {1, 2});
    assert((v2 == std::vector<Point>{{1, 2}, {1, 2}, {1, 2}}));

    std::vector<Point> v3(3);
    std::ranges::fill_n(v3.begin(), 3, {3, 4});
    assert((v3 == std::vector<Point>{{3, 4}, {3, 4}, {3, 4}}));
  }

  {
    std::vector<Point> v{{1, 2}, {3, 4}, {1, 2}};
    assert(std::ranges::remove(v.begin(), v.end(), {1, 2}).begin() == v.begin() + 1);

    std::vector<Point> v2{{1, 2}, {3, 4}, {1, 2}};
    assert(std::ranges::remove(v2, {1, 2}).begin() == v2.begin() + 1);

    std::vector<Point> src{{1, 2}, {3, 4}, {1, 2}};
    std::vector<Point> out(3);
    assert(std::ranges::remove_copy(src.begin(), src.end(), out.begin(), {1, 2}).out == out.begin() + 1);

    std::vector<Point> out2(3);
    assert(std::ranges::remove_copy(src, out2.begin(), {1, 2}).out == out2.begin() + 1);
  }

  {
    std::vector<Point> v{{1, 0}, {2, 0}, {2, 0}, {3, 0}};
    assert(std::ranges::lower_bound(v.begin(), v.end(), {2, 0}, LessByX()) == v.begin() + 1);
    assert(std::ranges::lower_bound(v, {2, 0}, LessByX()) == v.begin() + 1);
    assert(std::ranges::upper_bound(v.begin(), v.end(), {2, 0}, LessByX()) == v.begin() + 3);
    assert(std::ranges::upper_bound(v, {2, 0}, LessByX()) == v.begin() + 3);
    auto er = std::ranges::equal_range(v.begin(), v.end(), {2, 0}, LessByX());
    assert(er.begin() == v.begin() + 1 && er.end() == v.begin() + 3);
    auto er2 = std::ranges::equal_range(v, {2, 0}, LessByX());
    assert(er2.begin() == v.begin() + 1 && er2.end() == v.begin() + 3);
    assert(std::ranges::binary_search(v.begin(), v.end(), {2, 0}, LessByX()));
    assert(std::ranges::binary_search(v, {2, 0}, LessByX()));
  }

  // A projection whose result type differs from the range's value type: only
  // projected_value_t (not iter_value_t) gives the right default here.
  {
    std::vector<Point> v{{1, 2}, {3, 4}, {5, 6}};
    assert(std::ranges::find(v, {4}, [](const Point& p) { return p.y; }) == v.begin() + 1);
    assert(std::ranges::find(v.begin(), v.end(), {4}, &Point::y) == v.begin() + 1);
  }

  // vector::erase() -- vector is constexpr-friendly in this implementation.
  {
    std::vector<Point> v{{1, 2}, {3, 4}, {1, 2}};
    assert(std::erase(v, {1, 2}) == 2);
  }

  return true;
}

// deque/list/forward_list aren't constexpr-constructible in this implementation, so their
// erase() free functions -- which get the same `class U = T` default -- are exercised only
// at runtime. basic_string's erase() uses the character type directly (no aggregate to
// brace-init), so it's exercised via a char literal instead, matching the paper's
// `class U = charT` default.
void test_runtime_only() {
  std::deque<Point> d{{1, 2}, {3, 4}, {1, 2}};
  assert(std::erase(d, {1, 2}) == 2);

  std::list<Point> l{{1, 2}, {3, 4}, {1, 2}};
  assert(std::erase(l, {1, 2}) == 2);

  std::forward_list<Point> fl{{1, 2}, {3, 4}, {1, 2}};
  assert(std::erase(fl, {1, 2}) == 2);

  std::string s = "hello world";
  assert(std::erase(s, 'o') == 2);
}

#if _LIBCPP_HAS_EXPERIMENTAL_PSTL
// The ExecutionPolicy overloads of count/fill/fill_n/find/replace/replace_if/replace_copy_if
// get the same defaulted value-type parameter as their sequential counterparts (replace_copy
// is deliberately excluded by the paper, same as the sequential overload). Not constexpr, so
// runtime-only.
void test_pstl() {
  std::vector<Point> v(3);
  std::fill(std::execution::seq, v.begin(), v.end(), {1, 2});
  assert((v == std::vector<Point>{{1, 2}, {1, 2}, {1, 2}}));

  std::vector<Point> v2(3);
  std::fill_n(std::execution::seq, v2.begin(), 3, {3, 4});
  assert((v2 == std::vector<Point>{{3, 4}, {3, 4}, {3, 4}}));

  std::vector<Point> v3{{1, 2}, {3, 4}, {5, 6}};
  assert(std::find(std::execution::seq, v3.begin(), v3.end(), {3, 4}) == v3.begin() + 1);
  assert(std::count(std::execution::seq, v3.begin(), v3.end(), {3, 4}) == 1);

  std::vector<Point> v4{{1, 2}, {3, 4}};
  std::replace(std::execution::seq, v4.begin(), v4.end(), {1, 2}, {9, 9});
  assert((v4 == std::vector<Point>{{9, 9}, {3, 4}}));

  std::vector<Point> v5{{1, 2}, {3, 4}};
  std::replace_if(
      std::execution::seq, v5.begin(), v5.end(), [](const Point& p) { return p.x == 1; }, {7, 7});
  assert((v5 == std::vector<Point>{{7, 7}, {3, 4}}));

  std::vector<Point> src{{1, 2}, {3, 4}};
  std::vector<Point> dst(2);
  std::replace_copy_if(
      std::execution::seq, src.begin(), src.end(), dst.begin(), [](const Point& p) { return p.x == 1; }, {8, 8});
  assert((dst == std::vector<Point>{{8, 8}, {3, 4}}));
}
#endif // _LIBCPP_HAS_EXPERIMENTAL_PSTL

int main(int, char**) {
  test();
  static_assert(test());

  test_runtime_only();

#if _LIBCPP_HAS_EXPERIMENTAL_PSTL
  test_pstl();
#endif

  return 0;
}
