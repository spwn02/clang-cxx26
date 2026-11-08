//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// P2688R6 6.2: a value pattern compares with 'bool(subject == E)' whenever
// that expression is well-formed, including the integral conversions.

#include <cassert>

constexpr int from_int(int value) {
  return match (value) {
    case true => 1;
    case 0 => 0;
    case _ => 2;
  };
}

constexpr int from_unsigned(unsigned value) {
  return match (value) {
    case -1 => 1;
    case _ => 0;
  };
}

constexpr int from_long(long value) {
  return match (value) {
    case 7 => 1;
    case _ => 0;
  };
}

static_assert(from_int(1) == 1);
static_assert(from_int(0) == 0);
static_assert(from_int(5) == 2);
static_assert(from_unsigned(~0u) == 1);
static_assert(from_unsigned(3u) == 0);
static_assert(from_long(7) == 1);

int main(int, char**) {
  assert(from_int(1) == 1);
  assert(from_int(0) == 0);
  assert(from_int(5) == 2);
  assert(from_unsigned(~0u) == 1);
  assert(from_unsigned(3u) == 0);
  assert(from_long(7L) == 1);
  return 0;
}
