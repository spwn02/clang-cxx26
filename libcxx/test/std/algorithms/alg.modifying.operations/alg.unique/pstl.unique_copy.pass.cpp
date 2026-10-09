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

// template<class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
//   ForwardIterator2 unique_copy(ExecutionPolicy&& exec,
//                                ForwardIterator1 first, ForwardIterator1 last,
//                                ForwardIterator2 result);
// template<class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
//   ForwardIterator2 unique_copy(ExecutionPolicy&& exec,
//                                ForwardIterator1 first, ForwardIterator1 last,
//                                ForwardIterator2 result, BinaryPredicate pred);

#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>

#include "test_macros.h"
#include "test_execution_policies.h"
#include "test_iterators.h"
#include "type_algorithms.h"

EXECUTION_POLICY_SFINAE_TEST(unique_copy);

static_assert(sfinae_test_unique_copy<int, int*, int*, int*>);
static_assert(sfinae_test_unique_copy<int, int*, int*, int*, bool (*)(int, int)>);
static_assert(!sfinae_test_unique_copy<std::execution::parallel_policy, int*, int*, int*>);

template <class Iter1, class Iter2>
struct Test {
  template <class Policy>
  void operator()(Policy&& policy) {
    for (const int size : {0, 1, 2, 100, 350}) {
      std::vector<int> in(size);
      for (int i = 0; i != size; ++i)
        in[i] = i / 3;
      {
        std::vector<int> out(size, -1);
        decltype(auto) ret =
            std::unique_copy(policy, Iter1(std::data(in)), Iter1(std::data(in) + size), Iter2(std::data(out)));
        static_assert(std::is_same_v<decltype(ret), Iter2>);
        std::vector<int> expected;
        std::unique_copy(in.begin(), in.end(), std::back_inserter(expected));
        assert(base(ret) == std::data(out) + expected.size());
        assert(std::equal(expected.begin(), expected.end(), out.begin()));
      }
      {
        auto same_parity = [](int a, int b) { return (a % 2) == (b % 2); };
        std::vector<int> out(size, -1);
        decltype(auto) ret = std::unique_copy(
            policy, Iter1(std::data(in)), Iter1(std::data(in) + size), Iter2(std::data(out)), same_parity);
        static_assert(std::is_same_v<decltype(ret), Iter2>);
        std::vector<int> expected;
        std::unique_copy(in.begin(), in.end(), std::back_inserter(expected), same_parity);
        assert(base(ret) == std::data(out) + expected.size());
        assert(std::equal(expected.begin(), expected.end(), out.begin()));
      }
    }
  }
};

int main(int, char**) {
  types::for_each(types::forward_iterator_list<int*>{}, types::apply_type_identity{[](auto v) {
                    using Iter = typename decltype(v)::type;
                    types::for_each(
                        types::forward_iterator_list<int*>{},
                        TestIteratorWithPolicies< types::partial_instantiation<Test, Iter>::template apply>{});
                  }});

  return 0;
}
