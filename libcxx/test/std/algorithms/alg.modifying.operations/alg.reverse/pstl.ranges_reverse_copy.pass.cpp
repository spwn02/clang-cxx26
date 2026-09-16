//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <algorithm>

// P3179R9: parallel range algorithms.
// P3709R2: reverse_copy's execution-policy overload returns
// reverse_copy_truncated_result<S, I, O> (in_in_out_result), not reverse_copy_result<I, O>.

// template<class ExecutionPolicy, random_access_iterator I, sized_sentinel_for<I> S,
//          weakly_incrementable O>
//   requires indirectly_copyable<I, O>
//   reverse_copy_truncated_result<S, I, O>
//     reverse_copy(ExecutionPolicy&& exec, I first, S last, O result);
// template<class ExecutionPolicy, random_access_range R, weakly_incrementable O>
//   requires indirectly_copyable<iterator_t<R>, O> && sized_range<R>
//   reverse_copy_truncated_result<sentinel_t<R>, iterator_t<R>, O>
//     reverse_copy(ExecutionPolicy&& exec, R&& r, O result);

#include <algorithm>
#include <cassert>
#include <execution>
#include <type_traits>
#include <vector>

int main(int, char**) {
  // Iterator overload.
  {
    int in[]  = {1, 2, 3, 4};
    int out[4] = {};

    auto ret = std::ranges::reverse_copy(std::execution::par, in, in + 4, out);
    static_assert(std::is_same_v<decltype(ret), std::ranges::reverse_copy_truncated_result<int*, int*, int*>>);
    // Full completion (this fork's PSTL backend never truncates): in1 == last, in2 == first.
    assert(ret.in1 == in + 4);
    assert(ret.in2 == in);
    assert(ret.out == out + 4);

    int expected[] = {4, 3, 2, 1};
    assert(std::equal(out, out + 4, expected));
  }
  // Range overload.
  {
    std::vector<int> in  = {1, 2, 3, 4, 5};
    std::vector<int> out(5);

    auto ret = std::ranges::reverse_copy(std::execution::par, in, out.begin());
    static_assert(std::is_same_v<
                  decltype(ret),
                  std::ranges::reverse_copy_truncated_result<std::vector<int>::iterator, std::vector<int>::iterator,
                                                              std::vector<int>::iterator>>);
    assert(ret.in1 == in.end());
    assert(ret.in2 == in.begin());
    assert(ret.out == out.end());

    std::vector<int> expected = {5, 4, 3, 2, 1};
    assert(out == expected);
  }
  // Empty range.
  {
    std::vector<int> in;
    std::vector<int> out;
    auto ret = std::ranges::reverse_copy(std::execution::par, in, out.begin());
    assert(ret.in1 == in.end());
    assert(ret.in2 == in.begin());
    assert(ret.out == out.begin());
  }
  return 0;
}
