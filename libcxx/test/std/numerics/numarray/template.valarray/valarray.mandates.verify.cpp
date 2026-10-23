//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <valarray>

// [valarray.unary] Mandates that the indicated operator can be applied and its
// result is convertible to value_type (or bool for operator!).

#include <valarray>

struct NoUnaryOperators {};
struct NoBinaryOperators {};

struct NotConvertible {};
struct BadAddition {
  NotConvertible operator+(const BadAddition&) const { return {}; }
};

struct BadComparison {
  NotConvertible operator==(const BadComparison&) const { return {}; }
};

namespace adl_abs {
struct Number {};
Number abs(Number&&) { return {}; }
} // namespace adl_abs

void test() {
  std::valarray<NoUnaryOperators> values(1);
  // expected-error@*:* {{Mandates: unary + must return a value convertible to valarray's value type}}
  (void)+values;
  // expected-error@*:* {{Mandates: unary - must return a value convertible to valarray's value type}}
  (void)-values;
  // expected-error@*:* {{Mandates: unary ~ must return a value convertible to valarray's value type}}
  (void)~values;
  // expected-error@*:* {{Mandates: unary ! must return a value convertible to bool}}
  (void)!values;

  std::valarray<NoBinaryOperators> no_binary_left(1), no_binary_right(1);
  // expected-error@*:* {{Mandates: the indicated valarray binary operator must be applicable and return the required type}}
  (void)(no_binary_left + no_binary_right);

  std::valarray<BadAddition> bad_add_left(1), bad_add_right(1);
  // expected-error@*:* {{Mandates: the indicated valarray binary operator must be applicable and return the required type}}
  (void)(bad_add_left + bad_add_right);

  std::valarray<BadComparison> bad_compare_left(1), bad_compare_right(1);
  // expected-error@*:* {{Mandates: the indicated valarray binary operator must be applicable and return the required type}}
  (void)(bad_compare_left == bad_compare_right);

  // expected-error@*:* {{Mandates: the indicated valarray function must be applicable and return the required type}}
  (void)std::abs(values);

  std::valarray<adl_abs::Number> adl_values(1);
  auto adl_result = std::abs(adl_values);
  (void)adl_result[0];
}
