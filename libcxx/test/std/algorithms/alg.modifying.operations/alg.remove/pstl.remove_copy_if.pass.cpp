//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <algorithm>

// template<class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class Predicate>
//   ForwardIterator2 remove_copy_if(ExecutionPolicy&& exec,
//                                   ForwardIterator1 first, ForwardIterator1 last,
//                                   ForwardIterator2 result, Predicate pred);

#include <algorithm>
#include <cassert>
#include <vector>

#include "test_macros.h"
#include "test_execution_policies.h"
#include "test_iterators.h"
#include "type_algorithms.h"

EXECUTION_POLICY_SFINAE_TEST(remove_copy_if);

static_assert(sfinae_test_remove_copy_if<int, int*, int*, int*, bool (*)(int)>);
static_assert(!sfinae_test_remove_copy_if<std::execution::parallel_policy, int*, int*, int*, int>);

template <class Iter1, class Iter2>
struct Test {
  template <class Policy>
  void operator()(Policy&& policy) {
    auto is_one = [](int x) { return x == 1; };
    for (const int size : {0, 1, 2, 100, 350}) {
      std::vector<int> in(size);
      for (int i = 0; i != size; ++i)
        in[i] = i % 3;
      std::vector<int> out(size, -1);
      decltype(auto) ret = std::remove_copy_if(
          policy, Iter1(std::data(in)), Iter1(std::data(in) + size), Iter2(std::data(out)), is_one);
      static_assert(std::is_same_v<decltype(ret), Iter2>);
      std::vector<int> expected;
      std::remove_copy_if(in.begin(), in.end(), std::back_inserter(expected), is_one);
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
