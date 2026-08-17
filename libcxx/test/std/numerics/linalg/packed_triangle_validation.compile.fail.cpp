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

using extents = std::extents<size_t, 2, 2>;
using layout = std::linalg::layout_blas_packed<std::linalg::lower_triangle_t, std::linalg::column_major_t>;

int main(int, char**) {
  int matrix_data[] = {1, 2, 3};
  int vector_data[] = {1, 2};
  int output_data[] = {0, 0};
  std::mdspan<int, extents, layout> matrix(matrix_data, extents{});
  std::mdspan vector(vector_data, 2);
  std::mdspan output(output_data, 2);

  std::linalg::triangular_matrix_vector_product( // expected-error {{requires a packed matrix matching the triangle tag}}
      matrix, std::linalg::upper_triangle, std::linalg::explicit_diagonal, vector, output);
}
