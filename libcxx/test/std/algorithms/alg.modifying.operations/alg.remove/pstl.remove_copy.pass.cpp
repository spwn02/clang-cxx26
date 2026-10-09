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

// template<class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T>
//   ForwardIterator2 remove_copy(ExecutionPolicy&& exec,
//                                ForwardIterator1 first, ForwardIterator1 last,
//                                ForwardIterator2 result, const T& value);

#include <algorithm>
#include <cassert>
#include <vector>

#include "test_macros.h"
#include "test_execution_policies.h"
#include "test_iterators.h"
#include "type_algorithms.h"

EXECUTION_POLICY_SFINAE_TEST(remove_copy);

static_assert(sfinae_test_remove_copy<int, int*, int*, int*, int>);
static_assert(!sfinae_test_remove_copy<std::execution::parallel_policy, int*, int*, int*, int>);

template <class Iter1, class Iter2>
struct Test {
  template <class Policy>
  void operator()(Policy&& policy) {
    for (const int size : {0, 1, 2, 100, 350}) {
      std::vector<int> in(size);
      for (int i = 0; i != size; ++i)
        in[i] = i % 3;
      std::vector<int> out(size, -1);
      decltype(auto) ret = std::remove_copy(
          policy, Iter1(std::data(in)), Iter1(std::data(in) + size), Iter2(std::data(out)), 1);
      static_assert(std::is_same_v<decltype(ret), Iter2>);
      std::vector<int> expected;
      std::remove_copy(in.begin(), in.end(), std::back_inserter(expected), 1);
      assert(base(ret) == std::data(out) + expected.size());
      assert(std::equal(expected.begin(), expected.end(), out.begin()));
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
