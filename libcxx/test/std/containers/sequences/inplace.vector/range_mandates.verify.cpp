//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <array>
#include <inplace_vector>

struct NonAssignable {
  NonAssignable()                                = default;
  NonAssignable(const NonAssignable&)            = default;
  NonAssignable& operator=(const NonAssignable&) = delete;
};

void test() {
  std::array<NonAssignable, 0> empty{};
  std::inplace_vector<NonAssignable, 0> zero;
  // expected-error@*:* {{inplace_vector::assign_range requires assignable elements}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  zero.assign_range(empty);

  std::array<int, 1> one{};
  std::inplace_vector<int, 0> zero_int;
  // expected-error@*:* {{inplace_vector::assign_range: range too large for capacity}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  zero_int.assign_range(one);
  // expected-error@*:* {{inplace_vector(from_range_t, R&&): range too large for capacity}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  std::inplace_vector<int, 0> zero_from_range(std::from_range, one);

  std::array<int, 3> three{};
  std::inplace_vector<int, 2> two;
  // expected-error@*:* {{inplace_vector::assign_range: range too large for capacity}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  two.assign_range(three);
}
