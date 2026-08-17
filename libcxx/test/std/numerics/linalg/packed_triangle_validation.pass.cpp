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

int main(int, char**) {
  using extents      = std::extents<size_t, 2, 2>;
  using upper_layout = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::column_major_t>;
  using lower_layout = std::linalg::layout_blas_packed<std::linalg::lower_triangle_t, std::linalg::row_major_t>;

  int upper_data[]  = {2, 3, 4};
  int lower_data[]  = {2, 3, 4};
  int vector_data[] = {5, 7};
  int output_data[] = {0, 0};
  std::mdspan<int, extents, upper_layout> upper(upper_data, extents{});
  std::mdspan<int, extents, lower_layout> lower(lower_data, extents{});
  std::mdspan vector(vector_data, 2);
  std::mdspan output(output_data, 2);

  std::linalg::symmetric_matrix_vector_product(upper, std::linalg::upper_triangle, vector, output);
  if (output[0] != 31 || output[1] != 43)
    return 1;

  std::linalg::triangular_matrix_vector_product(
      lower, std::linalg::lower_triangle, std::linalg::explicit_diagonal, vector, output);
  if (output[0] != 10 || output[1] != 43)
    return 1;
}
