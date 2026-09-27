//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cassert>
#include <limits>
#include <sstream>
#include <stdfloat>

#include "test_macros.h"

template <class _Extended>
void test_roundtrip() {
  std::stringstream __stream;
  __stream << static_cast<_Extended>(1.5);
  assert(__stream.str() == "1.5");
  _Extended __value = 0;
  __stream >> __value;
  assert(!__stream.fail());
  assert(__value == static_cast<_Extended>(1.5));
}

template <class _Extended>
void test_overflow() {
  std::stringstream __stream("1e100");
  _Extended __value = 0;
  __stream >> __value;
  assert(__stream.fail());
  assert(__value == std::numeric_limits<_Extended>::max());
}

int main(int, char**) {
#if defined(__STDCPP_FLOAT16_T__)
  test_roundtrip<std::float16_t>();
  test_overflow<std::float16_t>();
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  test_roundtrip<std::bfloat16_t>();
  test_overflow<std::bfloat16_t>();
#endif
#if defined(__STDCPP_FLOAT32_T__)
  test_roundtrip<std::float32_t>();
#endif
#if defined(__STDCPP_FLOAT64_T__)
  test_roundtrip<std::float64_t>();
#endif
  return 0;
}
