//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <random>

struct ExplicitWeight {
  explicit operator double() const { return 1.0; }
};

void test() {
  ExplicitWeight weights[2];
  double doubles[2] = {1.0, 1.0};

  std::discrete_distribution<> discrete(weights, weights + 2);
  // expected-note@-1 {{in instantiation of function template specialization}}
  // expected-error@*:* {{Mandates: iterator_traits<InputIterator>::value_type is convertible to double}}

  std::piecewise_constant_distribution<> constant1(weights, weights + 2, doubles);
  // expected-note@-1 {{in instantiation of function template specialization}}
  // expected-error@*:* {{Mandates: iterator value types are convertible to double}}

  std::piecewise_linear_distribution<> linear1(weights, weights + 2, doubles);
  // expected-note@-1 {{in instantiation of function template specialization}}
  // expected-error@*:* {{Mandates: iterator value types are convertible to double}}
}
