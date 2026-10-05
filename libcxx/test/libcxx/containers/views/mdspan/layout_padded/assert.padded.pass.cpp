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

// [mdspan.layout.leftpad], [mdspan.layout.rightpad]: Preconditions of the padded layout mappings.

#include <array>
#include <cstddef>
#include <mdspan>

#include "check_assertion.h"

int main(int, char**) {
  using E = std::extents<int, std::dynamic_extent, 3>;
  using LeftStatic = std::layout_left_padded<4>::mapping<E>;
  using RightStatic = std::layout_right_padded<4>::mapping<E>;
  using LeftDyn = std::layout_left_padded<>::mapping<E>;
  using RightDyn = std::layout_right_padded<>::mapping<E>;

  // the runtime padding must equal a static padding_value
  TEST_LIBCPP_ASSERT_FAILURE(
      LeftStatic(E(2), 8), "layout_left_padded::mapping(extents, padding): padding must equal padding_value.");
  TEST_LIBCPP_ASSERT_FAILURE(
      RightStatic(E(2), 8), "layout_right_padded::mapping(extents, padding): padding must equal padding_value.");
  // the runtime padding must be positive
  TEST_LIBCPP_ASSERT_FAILURE(
      LeftDyn(E(2), 0),
      "layout_left_padded::mapping(extents, padding): padding must be representable as index_type and positive.");
  TEST_LIBCPP_ASSERT_FAILURE(
      RightDyn(E(2), -1),
      "layout_right_padded::mapping(extents, padding): padding must be representable as index_type and positive.");

  // layout_stride conversions need a unit inner stride
  std::layout_stride::mapping<E> strided(E(2), std::array<int, 2>{2, 8});
  TEST_LIBCPP_ASSERT_FAILURE(LeftDyn(strided), "layout_left_padded::mapping from layout_stride: other.stride(0) must be 1.");

  // indexing and stride() are bounds checked
  LeftStatic l(E(2));
  RightStatic r(E(2));
  TEST_LIBCPP_ASSERT_FAILURE(l(2, 0), "layout_left_padded::mapping: out of bounds indexing");
  TEST_LIBCPP_ASSERT_FAILURE(r(0, 3), "layout_right_padded::mapping: out of bounds indexing");
  TEST_LIBCPP_ASSERT_FAILURE(l.stride(2), "layout_left_padded::mapping::stride(): invalid rank index");
  TEST_LIBCPP_ASSERT_FAILURE(r.stride(2), "layout_right_padded::mapping::stride(): invalid rank index");

  // valid uses
  (void)LeftStatic(E(2), 4);
  (void)RightDyn(E(2), 7);
  return 0;
}
