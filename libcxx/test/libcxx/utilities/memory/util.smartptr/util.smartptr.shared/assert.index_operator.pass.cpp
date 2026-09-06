//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: has-unix-headers
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: libcpp-hardening-mode=none
// XFAIL: libcpp-hardening-mode=debug && availability-verbose_abort-missing

#include <cassert>
#include <memory>

#include "check_assertion.h"

int main(int, char**) {
  std::shared_ptr<int[5]> bounded = std::make_shared<int[5]>();
  for (int i = 0; i != 5; ++i)
    (void)bounded[i];
  TEST_LIBCPP_ASSERT_FAILURE(bounded[-1], "shared_ptr<T[]>::operator[] index out of bounds");
  TEST_LIBCPP_ASSERT_FAILURE(bounded[5], "shared_ptr<T[]>::operator[] index out of bounds");

  std::shared_ptr<int[]> unbounded = std::make_shared<int[]>(5);
  (void)unbounded[0];
  TEST_LIBCPP_ASSERT_FAILURE(unbounded[-1], "shared_ptr<T[]>::operator[] index out of bounds");

  return 0;
}
