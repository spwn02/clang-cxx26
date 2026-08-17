//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

#include <linalg>

#include <type_traits>

#include "test_macros.h"

template <class Layout>
constexpr bool test_packed() {
  using extents = std::extents<int, 3, 3>;
  using mapping = typename Layout::template mapping<extents>;
  constexpr mapping map{extents{}};

  static_assert(map.required_span_size() == 6);
  if (map(0, 0) != 0 || map(2, 2) != 5 || map(0, 2) != map(2, 0) || map(1, 2) != map(2, 1) || !map.is_exhaustive() ||
      map.is_unique())
    return false;
  return true;
}

constexpr bool test() {
  using upper_column = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::column_major_t>;
  using lower_column = std::linalg::layout_blas_packed<std::linalg::lower_triangle_t, std::linalg::column_major_t>;
  using upper_row    = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::row_major_t>;
  using lower_row    = std::linalg::layout_blas_packed<std::linalg::lower_triangle_t, std::linalg::row_major_t>;
  static_assert(test_packed<upper_column>());
  static_assert(test_packed<lower_column>());
  static_assert(test_packed<upper_row>());
  static_assert(test_packed<lower_row>());

  constexpr std::array<size_t, 6> upper_column_offsets   = {0, 1, 3, 2, 4, 5};
  constexpr std::array<size_t, 6> upper_row_offsets      = {0, 1, 2, 3, 4, 5};
  constexpr std::array<size_t, 6> lower_column_offsets   = {0, 1, 2, 3, 4, 5};
  constexpr std::array<size_t, 6> lower_row_offsets      = {0, 1, 3, 2, 4, 5};
  constexpr std::array<std::array<size_t, 2>, 6> indices = {{{0, 0}, {0, 1}, {0, 2}, {1, 1}, {1, 2}, {2, 2}}};
  constexpr upper_column::mapping<std::extents<size_t, 3, 3>> upper_column_mapping{std::extents<size_t, 3, 3>{}};
  constexpr upper_row::mapping<std::extents<size_t, 3, 3>> upper_row_mapping{std::extents<size_t, 3, 3>{}};
  constexpr lower_column::mapping<std::extents<size_t, 3, 3>> lower_column_mapping{std::extents<size_t, 3, 3>{}};
  constexpr lower_row::mapping<std::extents<size_t, 3, 3>> lower_row_mapping{std::extents<size_t, 3, 3>{}};
  for (size_t i = 0; i != indices.size(); ++i) {
    if (upper_column_mapping(indices[i][0], indices[i][1]) != upper_column_offsets[i] ||
        upper_row_mapping(indices[i][0], indices[i][1]) != upper_row_offsets[i] ||
        lower_column_mapping(indices[i][0], indices[i][1]) != lower_column_offsets[i] ||
        lower_row_mapping(indices[i][0], indices[i][1]) != lower_row_offsets[i])
      return false;
  }

  int dense_data[] = {0, 1, 2, 3, 4, 5};
  std::mdspan<int, std::extents<size_t, 2, 3>> dense(dense_data, std::extents<size_t, 2, 3>{});
  auto dense_t = std::linalg::transposed(dense);
  ASSERT_SAME_TYPE(typename decltype(dense_t)::layout_type, std::layout_left);
  static_assert(decltype(dense_t)::extents_type::static_extent(0) == 3);
  static_assert(decltype(dense_t)::extents_type::static_extent(1) == 2);
  if (dense_t[2, 1] != dense[1, 2])
    return false;

  int packed_data[]    = {0, 1, 2, 3, 4, 5};
  using packed_extents = std::extents<size_t, 3, 3>;
  std::mdspan<int, packed_extents, upper_column> packed(packed_data, packed_extents{});
  auto packed_t = std::linalg::transposed(packed);
  ASSERT_SAME_TYPE(typename decltype(packed_t)::layout_type, lower_row);
  if (packed_t[2, 0] != packed[0, 2] || packed_t[1, 2] != packed[2, 1])
    return false;
  auto packed_tt = std::linalg::transposed(packed_t);
  ASSERT_SAME_TYPE(typename decltype(packed_tt)::layout_type, upper_column);
  if (packed_tt[2, 1] != packed[2, 1])
    return false;

  using strided_extents = std::extents<size_t, 2, 3>;
  std::layout_stride::mapping<strided_extents> strided_mapping(strided_extents{}, std::array<size_t, 2>{4, 1});
  std::mdspan<int, strided_extents, std::layout_stride> strided(dense_data, strided_mapping);
  auto strided_t = std::linalg::transposed(strided);
  ASSERT_SAME_TYPE(typename decltype(strided_t)::layout_type, std::layout_stride);
  if (strided_t.stride(0) != 1 || strided_t.stride(1) != 4)
    return false;

  using nested_extents    = std::extents<size_t, 2, 3>;
  using transpose_extents = std::extents<size_t, 3, 2>;
  using transpose_layout  = std::linalg::layout_transpose<std::layout_right>;
  std::layout_right::mapping<nested_extents> nested_mapping(nested_extents{});
  typename transpose_layout::template mapping<transpose_extents> transpose_mapping(nested_mapping);
  if (transpose_mapping.required_span_size() != nested_mapping.required_span_size() ||
      transpose_mapping.stride(0) != nested_mapping.stride(1) ||
      transpose_mapping.stride(1) != nested_mapping.stride(0) || transpose_mapping(2, 1) != nested_mapping(1, 2))
    return false;
  return true;
}

int main(int, char**) {
  static_assert(test());
  return test() ? 0 : 1;
}
