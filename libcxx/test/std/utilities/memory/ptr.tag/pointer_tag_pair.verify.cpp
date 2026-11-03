//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <memory>

// P3125R6: the Mandates of std::pointer_tag_pair and the constant evaluation of an invalid use.

#include <memory>

void f();

void test_mandates() {
  // expected-error@*:* {{Ptr must be a pointer to an object or to void}}
  std::pointer_tag_pair<void (*)(), 1> function_pointer;
  // expected-error@*:* {{Ptr must be a pointer to an object or to void}}
  std::pointer_tag_pair<int, 1> not_a_pointer;
  // expected-error@*:* {{TagT must be an unsigned integer or an enum of one}}
  std::pointer_tag_pair<int*, 1, int> signed_tag;
  // expected-error@*:* {{More tag bits requested than a pointer can hold}}
  std::pointer_tag_pair<void*, 40> too_many_bits;
  // expected-error@*:* {{Ptr must not be cv-qualified or a reference}}
  std::pointer_tag_pair<int* const, 1> const_pointer;
  (void)function_pointer;
  (void)not_a_pointer;
  (void)signed_tag;
  (void)too_many_bits;
  (void)const_pointer;
}

// A use that cannot be a constant expression: the object is only aligned to its type's alignment.
alignas(4) constexpr int Four = 0;
constexpr std::pointer_tag_pair<const int*, 2> ok(&Four, 3u);
// expected-error@+2 {{constexpr variable 'too_big' must be initialized by a constant expression}}
// expected-note@*:* {{tag value 4 does not fit in 2 bit(s)}}
constexpr std::pointer_tag_pair<const int*, 2> too_big(&Four, 4u);

// The precondition of pointer_bits_available (a power of two) is checked in constant expressions in every hardening mode.
// expected-error@+1 {{constexpr variable 'not_a_power_of_two' must be initialized by a constant expression}}
constexpr unsigned not_a_power_of_two = std::pointer_bits_available(3);
// expected-error@+1 {{constexpr variable 'zero_alignment' must be initialized by a constant expression}}
constexpr unsigned zero_alignment = std::pointer_bits_available(0);

// from_overaligned: a promise the evaluator can disprove or cannot prove is not a constant expression.
alignas(4) constexpr int FourAligned = 0;
// expected-error@+1 {{constexpr variable 'broken_promise' must be initialized by a constant expression}}
constexpr auto broken_promise = std::pointer_tag_pair<const void*, 3>::from_overaligned<8>(&FourAligned, 1u);
