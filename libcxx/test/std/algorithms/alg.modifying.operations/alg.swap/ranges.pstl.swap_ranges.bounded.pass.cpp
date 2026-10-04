//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

#include <algorithm>
#include <cassert>
#include <execution>
#include <span>
#include <type_traits>

struct unsized_sentinel {
  int* last;
  friend bool operator==(int* current, unsized_sentinel bound) { return current == bound.last; }
};
struct sized_range {
  int* first;
  int count;
  int* begin() const { return first; }
  unsized_sentinel end() const { return {first + count}; }
  int size() const { return count; }
};
static_assert(std::ranges::random_access_range<sized_range> && std::ranges::sized_range<sized_range>);
static_assert(!std::sized_sentinel_for<unsized_sentinel, int*>);

// [alg.swap]: "M be min(last1 - first1, last2 - first2)."
// "For each non-negative integer n < M performs: ...
// ranges::iter_swap(first1 + n, first2 + n) ..."
// "{first1 + M, first2 + M} for the overloads in namespace ranges."
template <class Policy>
void test(Policy&& policy) {
  for (int length1 : {0, 2, 4}) {
    for (int length2 : {0, 2, 4}) {
      for (bool range_form : {false, true}) {
        int first[]  = {1, 2, 3, 4};
        int second[] = {9, 8, 7, 6};
        int count = length1 < length2 ? length1 : length2;
        auto check = [&](auto result) {
          assert(std::to_address(result.in1) == first + count);
          assert(std::to_address(result.in2) == second + count);
        };
        if (range_form) {
          check(std::ranges::swap_ranges(policy, std::span(first, length1), std::span(second, length2)));
        } else {
          check(std::ranges::swap_ranges(policy, first, first + length1, second, second + length2));
        }
        for (int index = 0; index < 4; ++index) {
          assert(first[index] == (index < count ? 9 - index : 1 + index));
          assert(second[index] == (index < count ? 1 + index : 9 - index));
        }
      }
    }
  }
  // [algorithms.requirements]: "a corresponding sentinel argument is initialized with
  // ranges::end(r), or ranges::begin(r) + N where N is equal to ranges::distance(r)."
  int first[] = {1, 2, 3, 4};
  int second[] = {9, 8};
  sized_range first_range{first, 4};
  sized_range second_range{second, 2};
  auto result = std::ranges::swap_ranges(policy, first_range, second_range);
  assert(result.in1 == first + 2 && result.in2 == second + 2);
  assert(first[0] == 9 && first[1] == 8 && first[2] == 3 && first[3] == 4);
  assert(second[0] == 1 && second[1] == 2);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
