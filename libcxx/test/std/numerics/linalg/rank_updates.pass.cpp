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
  int left_data[]   = {1, 2};
  int right_data[]  = {3, 4};
  int output_data[] = {0, 0, 0, 0};
  std::mdspan left(left_data, 2);
  std::mdspan right(right_data, 2);
  std::mdspan output(output_data, 2, 2);
  std::linalg::matrix_rank_1_update(left, right, output);
  if (output[0, 0] != 3 || output[0, 1] != 4 || output[1, 0] != 6 || output[1, 1] != 8)
    return 1;

  // The output is out-matrix, so a non-zero prior value must be overwritten,
  // not accumulated into. A zero-initialized output cannot tell the two apart.
  int dirty_data[] = {100, 200, 300, 400};
  std::mdspan dirty(dirty_data, 2, 2);
  std::linalg::matrix_rank_1_update(left, right, dirty);
  if (dirty[0, 0] != 3 || dirty[0, 1] != 4 || dirty[1, 0] != 6 || dirty[1, 1] != 8)
    return 1;

  int dirty_c_data[] = {100, 200, 300, 400};
  std::mdspan dirty_c(dirty_c_data, 2, 2);
  std::linalg::matrix_rank_1_update_c(left, right, dirty_c);
  if (dirty_c[0, 0] != 3 || dirty_c[0, 1] != 4 || dirty_c[1, 0] != 6 || dirty_c[1, 1] != 8)
    return 1;

  // The updating forms A = E + x*y^T; A may alias E.
  int e_data[]  = {10, 20, 30, 40};
  int au_data[] = {0, 0, 0, 0};
  std::mdspan e_view(e_data, 2, 2);
  std::mdspan au(au_data, 2, 2);
  std::linalg::matrix_rank_1_update(left, right, e_view, au);
  if (au[0, 0] != 13 || au[0, 1] != 24 || au[1, 0] != 36 || au[1, 1] != 48)
    return 1;

  std::linalg::matrix_rank_1_update_c(left, right, e_view, au);
  if (au[0, 0] != 13 || au[0, 1] != 24 || au[1, 0] != 36 || au[1, 1] != 48)
    return 1;

  int aliased_data[] = {10, 20, 30, 40};
  std::mdspan aliased(aliased_data, 2, 2);
  std::linalg::matrix_rank_1_update(left, right, aliased, aliased);
  if (aliased[0, 0] != 13 || aliased[0, 1] != 24 || aliased[1, 0] != 36 || aliased[1, 1] != 48)
    return 1;

  // Same check for the symmetric/hermitian rank-1 form, which also overwrites.
  int dirty_sym_data[] = {100, 200, 300, 400};
  std::mdspan dirty_sym(dirty_sym_data, 2, 2);
  std::linalg::symmetric_matrix_rank_1_update(2, left, dirty_sym, std::linalg::lower_triangle);
  // 2*x*x^T with x = {1,2} is {{2,4},{4,8}}; lower_triangle leaves [0,1] alone.
  if (dirty_sym[0, 0] != 2 || dirty_sym[0, 1] != 200 || dirty_sym[1, 0] != 4 || dirty_sym[1, 1] != 8)
    return 1;

  int symmetric_data[] = {10, 20, 30, 40};
  std::mdspan symmetric(symmetric_data, 2, 2);
  // [linalg.algs.blas2.rank2]: A = x*y^T + y*x^T, overwriting the triangle.
  // With x = {1,2} and y = {3,4} that is {{6,10},{10,16}}; upper_triangle
  // leaves [1,0] at its original 30.
  std::linalg::symmetric_matrix_rank_2_update(left, right, symmetric, std::linalg::upper_triangle);
  if (symmetric[0, 0] != 6 || symmetric[0, 1] != 10 || symmetric[1, 0] != 30 || symmetric[1, 1] != 16)
    return 1;

  int matrix_data[] = {1, 2, 3, 4};
  int rank_k_data[] = {10, 20, 30, 40};
  std::mdspan matrix(matrix_data, 2, 2);
  std::mdspan rank_k(rank_k_data, 2, 2);
  // [linalg.algs.blas3.rankk]: the overload without E computes C = alpha*A*A^T,
  // overwriting the specified triangle rather than accumulating into it.
  // A*A^T is {{5,11},{11,25}}, so 2*A*A^T is {{10,22},{22,50}}; with
  // lower_triangle only [0,0], [1,0] and [1,1] are written and [0,1] keeps its
  // original 20.
  std::linalg::symmetric_matrix_rank_k_update(2, matrix, rank_k, std::linalg::lower_triangle);
  if (rank_k[0, 0] != 10 || rank_k[0, 1] != 20 || rank_k[1, 0] != 22 || rank_k[1, 1] != 50)
    return 1;

  // [linalg.algs.blas3.rank2k]: C = A*B^T + B*A^T, also overwriting. With
  // A == B == matrix that is 2*A*A^T == {{10,22},{22,50}}; upper_triangle
  // leaves [1,0] at its original 30.
  int rank_2k_data[] = {10, 20, 30, 40};
  std::mdspan rank_2k(rank_2k_data, 2, 2);
  std::linalg::symmetric_matrix_rank_2k_update(matrix, matrix, rank_2k, std::linalg::upper_triangle);
  if (rank_2k[0, 0] != 10 || rank_2k[0, 1] != 22 || rank_2k[1, 0] != 30 || rank_2k[1, 1] != 50)
    return 1;

  using complex            = std::complex<int>;
  complex complex_data[]   = {{1, 2}, {3, -1}};
  complex hermitian_data[] = {{}, {}, {}, {}};
  std::mdspan complex_vector(complex_data, 2);
  std::mdspan hermitian(hermitian_data, 2, 2);
  std::linalg::hermitian_matrix_rank_1_update(2, complex_vector, hermitian, std::linalg::lower_triangle);
  if (hermitian[0, 0] != complex(10, 0) || hermitian[1, 0] != complex(2, -14) || hermitian[1, 1] != complex(20, 0))
    return 1;

  int add_data[]    = {1, 2, 3, 4};
  int result_data[] = {0, 0, 0, 0};
  std::mdspan add(add_data, 2, 2);
  std::mdspan result(result_data, 2, 2);
  std::linalg::symmetric_matrix_rank_1_update(2, left, add, result, std::linalg::lower_triangle);
  if (result[0, 0] != 3 || result[1, 0] != 7 || result[1, 1] != 12)
    return 1;
  // rank_1 likewise always takes alpha; there is no alpha-free overload.
  std::linalg::symmetric_matrix_rank_1_update(1, left, add, result, std::linalg::upper_triangle);
  std::linalg::symmetric_matrix_rank_2_update(left, right, add, result, std::linalg::upper_triangle);
  std::linalg::symmetric_matrix_rank_k_update(2, matrix, add, result, std::linalg::lower_triangle);
  // Every rank_k overload takes alpha; there is no alpha-free form in the
  // synopsis, so pass 1 explicitly.
  std::linalg::symmetric_matrix_rank_k_update(1, matrix, add, result, std::linalg::upper_triangle);
  std::linalg::symmetric_matrix_rank_2k_update(matrix, matrix, add, result, std::linalg::lower_triangle);

  complex complex_add_data[]    = {{1, 0}, {2, 3}, {4, -2}, {5, 0}};
  complex complex_result_data[] = {{}, {}, {}, {}};
  std::mdspan complex_add(complex_add_data, 2, 2);
  std::mdspan complex_result(complex_result_data, 2, 2);
  std::linalg::hermitian_matrix_rank_1_update(
      2, complex_vector, complex_add, complex_result, std::linalg::lower_triangle);
  if (complex_result[0, 0] != complex(11, 0) || complex_result[1, 0] != complex(6, -16) ||
      complex_result[1, 1] != complex(25, 0))
    return 1;
  std::linalg::hermitian_matrix_rank_1_update(
      1, complex_vector, complex_add, complex_result, std::linalg::upper_triangle);
  std::linalg::hermitian_matrix_rank_2_update(
      complex_vector, complex_vector, complex_add, complex_result, std::linalg::upper_triangle);

  // Only "C may alias E" is permitted; the input matrix A aliasing the output C
  // is a precondition violation, so these use a distinct input buffer.
  complex complex_input_data[] = {{1, 0}, {0, 1}, {0, -1}, {2, 0}};
  std::mdspan complex_input(complex_input_data, 2, 2);
  std::linalg::hermitian_matrix_rank_k_update(
      2, complex_input, complex_add, complex_result, std::linalg::lower_triangle);
  std::linalg::hermitian_matrix_rank_k_update(
      1, complex_input, complex_add, complex_result, std::linalg::upper_triangle);
  std::linalg::hermitian_matrix_rank_2k_update(
      complex_input, complex_input, complex_add, complex_result, std::linalg::lower_triangle);

  return 0;
}
