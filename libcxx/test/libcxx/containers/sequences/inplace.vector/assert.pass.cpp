//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <inplace_vector>

// Test hardening assertions for std::inplace_vector.

// REQUIRES: has-unix-headers
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-hardening-mode=none
// XFAIL: libcpp-hardening-mode=debug && availability-verbose_abort-missing

#include <inplace_vector>

#include "check_assertion.h"

int main(int, char**) {
  // N > 0: empty container.
  {
    std::inplace_vector<int, 4> c;
    TEST_LIBCPP_ASSERT_FAILURE(c.front(), "front() called on an empty inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(c.back(), "back() called on an empty inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(c[0], "inplace_vector[] index out of bounds");
    TEST_LIBCPP_ASSERT_FAILURE(c.pop_back(), "pop_back() called on an empty inplace_vector");

    const std::inplace_vector<int, 4>& cc = c;
    TEST_LIBCPP_ASSERT_FAILURE(cc.front(), "front() called on an empty inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(cc.back(), "back() called on an empty inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(cc[0], "inplace_vector[] index out of bounds");
  }

  // N > 0: out-of-bounds index on a non-empty container.
  {
    std::inplace_vector<int, 4> c{1, 2, 3};
    TEST_LIBCPP_ASSERT_FAILURE(c[3], "inplace_vector[] index out of bounds");
    TEST_LIBCPP_ASSERT_FAILURE(c[100], "inplace_vector[] index out of bounds");

    const std::inplace_vector<int, 4>& cc = c;
    TEST_LIBCPP_ASSERT_FAILURE(cc[3], "inplace_vector[] index out of bounds");
    TEST_LIBCPP_ASSERT_FAILURE(cc[100], "inplace_vector[] index out of bounds");
  }

  // N == 0: the zero-capacity specialization -- every accessor has an
  // unsatisfiable precondition (size() is always 0).
  {
    std::inplace_vector<int, 0> c;
    TEST_LIBCPP_ASSERT_FAILURE(
        c.front(), "cannot call inplace_vector<T, 0>::front() on a zero-sized inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(c.back(), "cannot call inplace_vector<T, 0>::back() on a zero-sized inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(
        c[0], "cannot call inplace_vector<T, 0>::operator[] on a zero-sized inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(
        c.pop_back(), "cannot call inplace_vector<T, 0>::pop_back() on a zero-sized inplace_vector");

    const std::inplace_vector<int, 0>& cc = c;
    TEST_LIBCPP_ASSERT_FAILURE(
        cc.front(), "cannot call inplace_vector<T, 0>::front() on a zero-sized inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(cc.back(), "cannot call inplace_vector<T, 0>::back() on a zero-sized inplace_vector");
    TEST_LIBCPP_ASSERT_FAILURE(
        cc[0], "cannot call inplace_vector<T, 0>::operator[] on a zero-sized inplace_vector");
  }

  return 0;
}
