//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// REQUIRES: with-pstl

// <algorithm>

// template<class ExecutionPolicy, class ForwardIterator>
//   ForwardIterator is_sorted_until(ExecutionPolicy&& exec, ForwardIterator first, ForwardIterator last);
// template<class ExecutionPolicy, class ForwardIterator, class Compare>
//   ForwardIterator is_sorted_until(ExecutionPolicy&& exec, ForwardIterator first, ForwardIterator last,
//                                   Compare comp);

#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>

#include "test_macros.h"
#include "test_execution_policies.h"
#include "test_iterators.h"
#include "type_algorithms.h"

EXECUTION_POLICY_SFINAE_TEST(is_sorted_until);

static_assert(sfinae_test_is_sorted_until<int, int*, int*>);
static_assert(sfinae_test_is_sorted_until<int, int*, int*, bool (*)(int, int)>);
static_assert(!sfinae_test_is_sorted_until<std::execution::parallel_policy, int*, int*, int>);

template <class Iter>
struct Test {
  template <class Policy>
  void operator()(Policy&& policy) {
    for (const int size : {0, 1, 2, 100, 350}) {
      for (const int break_at : {0, 1, 50, 349, 1000}) {
        std::vector<int> in(size);
        for (int i = 0; i != size; ++i)
          in[i] = i;
        if (break_at > 0 && break_at < size)
          in[break_at] = -1;
        {
          decltype(auto) ret = std::is_sorted_until(policy, Iter(std::data(in)), Iter(std::data(in) + size));
          static_assert(std::is_same_v<decltype(ret), Iter>);
          assert(base(ret) == std::data(in) + (std::is_sorted_until(in.begin(), in.end()) - in.begin()));
        }
        {
          decltype(auto) ret =
              std::is_sorted_until(policy, Iter(std::data(in)), Iter(std::data(in) + size), std::less<int>{});
          assert(base(ret) == std::data(in) + (std::is_sorted_until(in.begin(), in.end()) - in.begin()));
        }
        {
          decltype(auto) ret =
              std::is_sorted_until(policy, Iter(std::data(in)), Iter(std::data(in) + size), std::greater<int>{});
          assert(base(ret) ==
                 std::data(in) + (std::is_sorted_until(in.begin(), in.end(), std::greater<int>{}) - in.begin()));
        }
      }
    }
  }
};

int main(int, char**) {
  types::for_each(types::forward_iterator_list<int*>{},
                  TestIteratorWithPolicies< types::partial_instantiation<Test>::template apply>{});
  return 0;
}
