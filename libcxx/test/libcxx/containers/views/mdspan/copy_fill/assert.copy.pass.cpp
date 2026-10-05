//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-hardening-mode=none

// <mdspan>

// [mdspan.copy]: copy(src, dst) hard-expects src.extents() == dst.extents() and expects dst.is_unique().

#include <mdspan>

#include "check_assertion.h"

int main(int, char**) {
  int a[12] = {}, b[12] = {};
  std::mdspan<int, std::dextents<int, 2>> s(a, 3, 4), d(b, 4, 3);
  TEST_LIBCPP_ASSERT_FAILURE(
      std::copy(s, d), "std::copy(mdspan): the source and destination extents must be equal");

  std::mdspan<int, std::dextents<int, 2>> ok(b, 3, 4);
  std::copy(s, ok);
  return 0;
}
