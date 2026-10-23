//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <numeric>

// Mandates: the result of binary_op(std::move(acc), *i) is implicitly convertible
// to InputIterator's value type.

#include <numeric>

struct Result {};
struct Value {
  Value& operator=(Result) { return *this; }
};
Result operator+(Value&&, const Value&) { return {}; }

void test() {
  Value input[2]{};
  Value output[2]{};
  // Assignment accepts Result, but Result is not implicitly convertible to Value.
  // expected-error@*:* {{Mandates: the result of the addition is implicitly convertible to InputIterator's value type}}
  (void)std::partial_sum(input, input + 2, output);
  // expected-error@*:* {{Mandates: the result of binary_op is implicitly convertible to InputIterator's value type}}
  (void)std::partial_sum(input, input + 2, output, [](Value&&, const Value&) { return Result{}; });
}
