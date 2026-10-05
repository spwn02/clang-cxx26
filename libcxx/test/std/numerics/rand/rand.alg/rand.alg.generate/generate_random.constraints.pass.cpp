//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <random>

// [alg.rand.generate]: the overloads taking a distribution require invoke_result_t<D&, G&> to be an arithmetic type.

#include <random>
#include <string>
#include <vector>

struct StringDistribution {
  std::string operator()(std::mt19937&) const { return {}; }
};

template <class Range, class Distribution>
concept can_generate_range = requires(Range& r, std::mt19937 g, Distribution d) {
  std::ranges::generate_random(r, g, d);
};

template <class Iterator, class Distribution>
concept can_generate_iterators = requires(Iterator f, Iterator l, std::mt19937 g, Distribution d) {
  std::ranges::generate_random(f, l, g, d);
};

static_assert(can_generate_range<std::vector<int>, std::uniform_int_distribution<int>>);
static_assert(can_generate_iterators<int*, std::uniform_int_distribution<int>>);
static_assert(!can_generate_range<std::vector<std::string>, StringDistribution>);
static_assert(!can_generate_iterators<std::string*, StringDistribution>);

int main(int, char**) { return 0; }
