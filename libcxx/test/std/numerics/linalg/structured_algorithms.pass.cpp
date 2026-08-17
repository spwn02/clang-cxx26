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

#include <complex>

int main(int, char**) {
  int triangular_data[] = {99, 99, 2, 99};
  int input_data[]      = {4, 5};
  int output_data[]     = {0, 0};
  std::mdspan triangular(triangular_data, 2, 2);
  std::mdspan input(input_data, 2);
  std::mdspan output(output_data, 2);
  std::linalg::triangular_matrix_vector_product(
      triangular, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, input, output);
  if (output[0] != 4 || output[1] != 13)
    return 1;
  std::linalg::triangular_matrix_vector_product(
      triangular, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, input);
  if (input[0] != 4 || input[1] != 13)
    return 1;

  int symmetric_data[] = {2, 3, 99, 4};
  int vector_data[]    = {5, 7};
  int result_data[]    = {0, 0};
  std::mdspan symmetric(symmetric_data, 2, 2);
  std::mdspan vector(vector_data, 2);
  std::mdspan result(result_data, 2);
  std::linalg::symmetric_matrix_vector_product(symmetric, std::linalg::upper_triangle, vector, result);
  if (result[0] != 31 || result[1] != 43)
    return 1;
  int add_data[] = {1, 2};
  std::mdspan add(add_data, 2);
  std::linalg::symmetric_matrix_vector_product(symmetric, std::linalg::upper_triangle, vector, add, result);
  if (result[0] != 32 || result[1] != 45)
    return 1;

  using complex                   = std::complex<int>;
  complex hermitian_data[]        = {{2, 0}, {1, 3}, {99, 99}, {4, 0}};
  complex hermitian_vector_data[] = {{1, 2}, {3, -1}};
  complex hermitian_result_data[] = {{}, {}};
  std::mdspan hermitian(hermitian_data, 2, 2);
  std::mdspan hermitian_vector(hermitian_vector_data, 2);
  std::mdspan hermitian_result(hermitian_result_data, 2);
  std::linalg::hermitian_matrix_vector_product(
      hermitian, std::linalg::upper_triangle, hermitian_vector, hermitian_result);
  if (hermitian_result[0] != complex(8, 12) || hermitian_result[1] != complex(19, -5))
    return 1;

  int other_data[]   = {1, 2, 3, 4};
  int product_data[] = {0, 0, 0, 0};
  std::mdspan other(other_data, 2, 2);
  std::mdspan product(product_data, 2, 2);
  std::linalg::symmetric_matrix_product(symmetric, std::linalg::upper_triangle, other, product);
  if (product[0, 0] != 11 || product[0, 1] != 16 || product[1, 0] != 15 || product[1, 1] != 22)
    return 1;
  int matrix_add_data[] = {1, 2, 3, 4};
  std::mdspan matrix_add(matrix_add_data, 2, 2);
  std::linalg::symmetric_matrix_product(symmetric, std::linalg::upper_triangle, other, matrix_add, product);
  if (product[0, 0] != 12 || product[0, 1] != 18 || product[1, 0] != 18 || product[1, 1] != 26)
    return 1;

  return 0;
}
