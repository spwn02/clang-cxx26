//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <numeric>

struct accumulator {};
struct input {};

struct unary {
  input operator()(input&) const { return {}; }
};
struct binary_transform {
  input operator()(input&, input&) const { return {}; }
};
struct only_accumulating {
  accumulator operator()(accumulator&& a, input) const { return a; }
};

accumulator unary_case(input* p) {
  return std::transform_reduce(p, p, accumulator{}, only_accumulating{}, unary{}); // expected-error@*:* {{transform_reduce requires all four binary_op argument combinations to produce T}} expected-note {{in instantiation of function template specialization}}
}

accumulator binary_case(input* p) {
  return std::transform_reduce(p, p, p, accumulator{}, only_accumulating{}, binary_transform{}); // expected-error@*:* {{transform_reduce requires all four binary_op1 argument combinations to produce T}} expected-note {{in instantiation of function template specialization}}
}
