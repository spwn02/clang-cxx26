//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: libcpp-hardening-mode=debug
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <mdspan>

// [mdspan.sub.canonical], [mdspan.sub.helpers], [mdspan.sub.map.common]: Expects of the slices (valid submdspan slices,
// representable indices, positive strides).

#include <mdspan>
#include <utility>

#include "check_assertion.h"

int main(int, char**) {
  using E = std::extents<int, std::dynamic_extent>;
  E e(5);

  // an index must be less than the extent
  TEST_LIBCPP_ASSERT_FAILURE(
      std::canonical_slices(e, 5),
      "canonical_slices: every slice must be a valid submdspan slice for the corresponding extent");
  TEST_LIBCPP_ASSERT_FAILURE(
      std::canonical_slices(e, -1),
      "canonical_slices: every slice must be a valid submdspan slice for the corresponding extent");
  // a range must lie inside the extent
  TEST_LIBCPP_ASSERT_FAILURE(
      std::canonical_slices(e, std::pair{2, 7}),
      "canonical_slices: every slice must be a valid submdspan slice for the corresponding extent");
  TEST_LIBCPP_ASSERT_FAILURE(
      std::canonical_slices(e, std::extent_slice{1, 3, 2}),
      "canonical_slices: every slice must be a valid submdspan slice for the corresponding extent");
  // the stride of a slice with more than one element must be positive
  TEST_LIBCPP_ASSERT_FAILURE(
      std::canonical_slices(e, std::extent_slice{0, 2, 0}),
      "canonical_slices: every slice must be a valid submdspan slice for the corresponding extent");
  TEST_LIBCPP_ASSERT_FAILURE(std::canonical_slices(e, std::range_slice{0, 4, 0}),
                             "submdspan: the stride of a range slice must be greater than zero");

  // an index must be representable as an index_type
  TEST_LIBCPP_ASSERT_FAILURE(std::canonical_slices(std::extents<unsigned, std::dynamic_extent>(5u), -1),
                             "submdspan: a slice index must be representable as a value of the index_type");

  // submdspan_mapping checks its slices as well
  std::layout_right::mapping<E> m(e);
  TEST_LIBCPP_ASSERT_FAILURE(
      submdspan_mapping(m, std::extent_slice{3, 3, 1}),
      "submdspan_mapping: every slice must be a valid submdspan slice for the corresponding extent");

  // valid slices, including an empty slice at the end, are accepted
  (void)std::canonical_slices(e, std::pair{5, 5});
  (void)std::canonical_slices(e, std::extent_slice{5, 0, 1});
  (void)std::canonical_slices(e, std::extent_slice{0, 3, 2});
  (void)submdspan_mapping(m, std::extent_slice{5, 0, 1});
  return 0;
}
