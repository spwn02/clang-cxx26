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
struct combine {
  accumulator operator()(accumulator, input) const { return {}; }
};
struct transform {
  input operator()(input) const { return {}; }
};

void exclusive(input* p, accumulator* out) {
  std::exclusive_scan(p, p, out, accumulator{}, combine{}); // expected-error@*:* {{exclusive_scan requires all three binary_op argument combinations to produce T}} expected-note {{in instantiation of function template specialization}}
}

void inclusive(input* p, accumulator* out) {
  std::inclusive_scan(p, p, out, combine{}, accumulator{}); // expected-error@*:* {{inclusive_scan requires all three binary_op argument combinations to produce T}} expected-note {{in instantiation of function template specialization}}
}

void transform_exclusive(input* p, accumulator* out) {
  std::transform_exclusive_scan(p, p, out, accumulator{}, combine{}, transform{}); // expected-error@*:* {{transform_exclusive_scan requires all three binary_op argument combinations to produce T}} expected-note {{in instantiation of function template specialization}}
}

void transform_inclusive(input* p, accumulator* out) {
  std::transform_inclusive_scan(p, p, out, combine{}, transform{}, accumulator{}); // expected-error@*:* {{transform_inclusive_scan requires all three binary_op argument combinations to produce T}} expected-note {{in instantiation of function template specialization}}
}
