//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <algorithm>

// [alg.search], [alg.remove], [alg.fill], [alg.replace]: the ExecutionPolicy overloads
//   - search_n has the overload without a predicate, and the value type T defaults to
//     iterator_traits<ForwardIterator>::value_type (P2248R8, so that {value} deduces),
//   - remove and remove_copy default T,
//   - fill_n returns ForwardIterator, replace_copy and replace_copy_if return ForwardIterator2.

#include <algorithm>
#include <cassert>
#include <concepts>
#include <execution>
#include <functional>
#include <ranges>
#include <span>

void test() {
  {
    int a[]{1, 2, 2, 3, 2, 2, 2};
    // search_n without a predicate, and a braced value
    assert(std::search_n(std::execution::seq, a, a + 7, 2, 2) == a + 1);
    assert(std::search_n(std::execution::seq, a, a + 7, 3, {2}) == a + 4);
    assert(std::search_n(std::execution::seq, a, a + 7, 2, {2}, std::equal_to<>{}) == a + 1);
  }

  {
    int a[]{1, 2, 3, 2, 4};
    int* end = std::remove(std::execution::seq, a, a + 5, {2});
    assert(end == a + 3);
    assert(a[0] == 1 && a[1] == 3 && a[2] == 4);
  }

  {
    int a[]{1, 2, 3, 2, 4};
    int b[5]{};
    int* end = std::remove_copy(std::execution::seq, a, a + 5, b, {2});
    assert(end == b + 3);
    assert(b[0] == 1 && b[1] == 3 && b[2] == 4);
  }

  {
    int a[4]{};
    static_assert(std::same_as<decltype(std::fill_n(std::execution::seq, a, 4, 7)), int*>);
    int* end = std::fill_n(std::execution::seq, a, 4, 7);
    assert(end == a + 4);
    assert(a[0] == 7 && a[3] == 7);
    assert(std::fill_n(std::execution::seq, a, 0, 9) == a);
    assert(a[0] == 7);
  }

  {
    int a[]{1, 2, 3};
    int b[3]{};
    static_assert(
        std::same_as<decltype(std::replace_copy_if(std::execution::seq, a, a + 3, b, [](int x) { return x == 2; }, 9)),
                     int*>);
    int* end = std::replace_copy_if(std::execution::seq, a, a + 3, b, [](int x) { return x == 2; }, 9);
    assert(end == b + 3);
    assert(b[0] == 1 && b[1] == 9 && b[2] == 3);
  }

  {
    int a[]{1, 2, 3};
    int b[3]{};
    static_assert(std::same_as<decltype(std::replace_copy(std::execution::seq, a, a + 3, b, 2, 9)), int*>);
    int* end = std::replace_copy(std::execution::seq, a, a + 3, b, 2, 9);
    assert(end == b + 3);
    assert(b[0] == 1 && b[1] == 9 && b[2] == 3);
  }
}

// [alg.partitions]: the ranges overloads of stable_partition with a policy are constrained by
// indirect_unary_predicate (a pointer is not a predicate, so the call is not viable and the
// iterator-sentinel overload is not an ambiguous choice).
template <class... Args>
concept can_stable_partition = requires(Args&&... args) { std::ranges::stable_partition(std::execution::seq, args...); };

void test_constraints() {
  int a[4]{1, 2, 3, 4};
  auto is_even = [](int x) { return x % 2 == 0; };
  static_assert(can_stable_partition<int*, int*, decltype(is_even)>);
  static_assert(can_stable_partition<std::span<int>, decltype(is_even)>);
  static_assert(!can_stable_partition<int*, int*, int*>);
  static_assert(!can_stable_partition<std::span<int>, int*>);
  auto sub = std::ranges::stable_partition(std::execution::seq, a, is_even);
  assert(sub.begin() == a + 2);
  assert(a[0] == 2 && a[1] == 4 && a[2] == 1 && a[3] == 3);
}

int main(int, char**) {
  test();
  test_constraints();
  return 0;
}
