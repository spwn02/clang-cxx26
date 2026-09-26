//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <numeric>

// P0452R1: execution-policy overloads of inclusive_scan, exclusive_scan, transform_inclusive_scan,
// transform_exclusive_scan and adjacent_difference.

#include <cassert>
#include <execution>
#include <functional>
#include <numeric>
#include <vector>

#include "test_execution_policies.h"
#include "test_iterators.h"
#include "test_macros.h"

template <class Iter>
struct Test {
  template <class Policy>
  void operator()(Policy&& policy) {
    for (int size : {0, 1, 2, 5, 100, 350}) {
      std::vector<int> in(size);
      for (int i = 0; i != size; ++i)
        in[i] = i + 1;

      auto check = [&](auto&& run, auto&& serial) {
        std::vector<int> got(size + 1, -1), expected(size + 1, -1);
        auto* got_end      = run(Iter(in.data()), Iter(in.data() + size), got.data());
        auto* expected_end = serial(in.data(), in.data() + size, expected.data());
        assert(got_end - got.data() == expected_end - expected.data());
        assert(got == expected);
      };

      // inclusive_scan
      check([&](auto f, auto l, int* o) { return std::inclusive_scan(policy, f, l, o); },
            [&](auto f, auto l, int* o) { return std::inclusive_scan(f, l, o); });
      check([&](auto f, auto l, int* o) { return std::inclusive_scan(policy, f, l, o, std::plus<>{}); },
            [&](auto f, auto l, int* o) { return std::inclusive_scan(f, l, o, std::plus<>{}); });
      check([&](auto f, auto l, int* o) { return std::inclusive_scan(policy, f, l, o, std::plus<>{}, 1000); },
            [&](auto f, auto l, int* o) { return std::inclusive_scan(f, l, o, std::plus<>{}, 1000); });
      // exclusive_scan
      check([&](auto f, auto l, int* o) { return std::exclusive_scan(policy, f, l, o, 7); },
            [&](auto f, auto l, int* o) { return std::exclusive_scan(f, l, o, 7); });
      check([&](auto f, auto l, int* o) { return std::exclusive_scan(policy, f, l, o, 7, std::plus<>{}); },
            [&](auto f, auto l, int* o) { return std::exclusive_scan(f, l, o, 7, std::plus<>{}); });
      // transform_inclusive_scan
      auto square = [](int x) { return x * x; };
      check([&](auto f, auto l, int* o) { return std::transform_inclusive_scan(policy, f, l, o, std::plus<>{}, square); },
            [&](auto f, auto l, int* o) { return std::transform_inclusive_scan(f, l, o, std::plus<>{}, square); });
      check([&](auto f, auto l, int* o) {
              return std::transform_inclusive_scan(policy, f, l, o, std::plus<>{}, square, 3);
            },
            [&](auto f, auto l, int* o) { return std::transform_inclusive_scan(f, l, o, std::plus<>{}, square, 3); });
      // transform_exclusive_scan
      check([&](auto f, auto l, int* o) {
              return std::transform_exclusive_scan(policy, f, l, o, 5, std::plus<>{}, square);
            },
            [&](auto f, auto l, int* o) { return std::transform_exclusive_scan(f, l, o, 5, std::plus<>{}, square); });
      // adjacent_difference
      check([&](auto f, auto l, int* o) { return std::adjacent_difference(policy, f, l, o); },
            [&](auto f, auto l, int* o) { return std::adjacent_difference(f, l, o); });
      check([&](auto f, auto l, int* o) { return std::adjacent_difference(policy, f, l, o, std::plus<>{}); },
            [&](auto f, auto l, int* o) { return std::adjacent_difference(f, l, o, std::plus<>{}); });
    }
  }
};

int main(int, char**) {
  types::for_each(types::forward_iterator_list<int*>{}, types::apply_type_identity{[](auto v) {
                    using Iter = typename decltype(v)::type;
                    TestIteratorWithPolicies<Test>{}.template operator()<Iter>();
                  }});

  // A known-good result for each algorithm.
  {
    std::vector<int> in{1, 2, 3, 4, 5}, out(5);
    std::inclusive_scan(std::execution::par, in.begin(), in.end(), out.begin());
    assert((out == std::vector<int>{1, 3, 6, 10, 15}));
    std::exclusive_scan(std::execution::par_unseq, in.begin(), in.end(), out.begin(), 100);
    assert((out == std::vector<int>{100, 101, 103, 106, 110}));
    std::adjacent_difference(std::execution::par, in.begin(), in.end(), out.begin());
    assert((out == std::vector<int>{1, 1, 1, 1, 1}));
  }
  return 0;
}
