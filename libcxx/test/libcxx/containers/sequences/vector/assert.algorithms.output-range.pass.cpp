//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <algorithm>

// A bounded vector iterator used as the output of a memmove-optimized algorithm must not be unwrapped to a raw pointer:
// the write to an output range that is larger than the container has to be diagnosed *before* any element is written
// (#157).

// REQUIRES: has-unix-headers
// UNSUPPORTED: libcpp-hardening-mode=none, c++03
// (Empty definitions: identical to the ones of a __config_site that already enables the bounded iterators.)
// ADDITIONAL_COMPILE_FLAGS: -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_VECTOR= -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STD_ARRAY=

#include <algorithm>
#include <array>
#include <vector>

#include "check_assertion.h"
#include "fill_to_capacity.h"

#define DEREF_MSG "__bounded_iter::operator*: Attempt to dereference an iterator at the end"

int main(int, char**) {
  std::vector<int> src(10, 7);
  std::vector<int> dst(2);
  fill_to_capacity(dst);

  TEST_LIBCPP_ASSERT_FAILURE(std::copy(src.begin(), src.end(), dst.begin()), DEREF_MSG);
  TEST_LIBCPP_ASSERT_FAILURE(std::copy_n(src.begin(), 10, dst.begin()), DEREF_MSG);
  TEST_LIBCPP_ASSERT_FAILURE(std::move(src.begin(), src.end(), dst.begin()), DEREF_MSG);
  TEST_LIBCPP_ASSERT_FAILURE(
      std::copy_backward(src.begin(), src.end(), dst.end()),
      "__bounded_iter::operator--: Attempt to rewind an iterator past the start");
  TEST_LIBCPP_ASSERT_FAILURE(std::fill_n(dst.begin(), 10, 1), DEREF_MSG);

  std::array<int, 2> arr = {};
  TEST_LIBCPP_ASSERT_FAILURE(
      std::copy(src.begin(), src.end(), arr.begin()),
      "__static_bounded_iter::operator*: Attempt to dereference an iterator at the end");

  // In-bounds ranges still work.
  std::copy(src.begin(), src.begin() + 2, dst.begin());
  return dst[0] == 7 && dst[1] == 7 ? 0 : 1;
}
