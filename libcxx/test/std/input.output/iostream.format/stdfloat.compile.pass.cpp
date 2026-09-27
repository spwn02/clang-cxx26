//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <istream>
#include <ostream>
#include <stdfloat>
#include <type_traits>

#include "test_macros.h"

template <class _Extended>
void test() {
  using _Output = std::ostream& (std::ostream::*)(_Extended);
  using _Input  = std::istream& (std::istream::*)(_Extended&);
  static_assert(std::is_same_v<decltype(static_cast<_Output>(&std::ostream::operator<<)), _Output>);
  static_assert(std::is_same_v<decltype(static_cast<_Input>(&std::istream::operator>>)), _Input>);
}

void test_all() {
#if defined(__STDCPP_FLOAT16_T__)
  test<std::float16_t>();
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  test<std::bfloat16_t>();
#endif
#if defined(__STDCPP_FLOAT32_T__)
  test<std::float32_t>();
#endif
#if defined(__STDCPP_FLOAT64_T__)
  test<std::float64_t>();
#endif
}
