//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <algorithm>

// template <class PopulationIterator, class SampleIterator, class Distance,
//           class UniformRandomNumberGenerator>
// SampleIterator sample(PopulationIterator first, PopulationIterator last,
//                       SampleIterator out, Distance n,
//                       UniformRandomNumberGenerator&& g);
// Mandates: Distance is an integer type.

#include <algorithm>
#include <random>

struct ConvertibleToInteger {
  operator int() const { return 1; }
};

void test() {
  int population[] = {1, 2};
  int sample[1]{};
  std::minstd_rand generator;
  // expected-error@*:* {{Mandates: Distance is an integer type}}
  (void)std::sample(population, population + 2, sample, ConvertibleToInteger{}, generator);
  // bool satisfies is_integral but is not a signed or unsigned integer type.
  // expected-error@*:* {{Mandates: Distance is an integer type}}
  (void)std::sample(population, population + 2, sample, true, generator);
}
