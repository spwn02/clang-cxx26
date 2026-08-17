// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std::linalg {
#if _LIBCPP_STD_VER >= 26
  // [linalg.tags.order], storage order tags
  using std::linalg::column_major;
  using std::linalg::column_major_t;
  using std::linalg::row_major;
  using std::linalg::row_major_t;

  // [linalg.tags.triangle], triangle tags
  using std::linalg::lower_triangle;
  using std::linalg::lower_triangle_t;
  using std::linalg::upper_triangle;
  using std::linalg::upper_triangle_t;

  // [linalg.tags.diagonal], diagonal tags
  using std::linalg::explicit_diagonal;
  using std::linalg::explicit_diagonal_t;
  using std::linalg::implicit_unit_diagonal;
  using std::linalg::implicit_unit_diagonal_t;

  // [linalg.layout.packed], class template layout_blas_packed
  using std::linalg::layout_blas_packed;

  // [linalg.scaled], scaled in-place transformation
  using std::linalg::scaled;
  using std::linalg::scaled_accessor;

  // [linalg.conj], conjugated in-place transformation
  using std::linalg::conjugated;
  using std::linalg::conjugated_accessor;

  // [linalg.transp], transpose in-place transformation
  using std::linalg::layout_transpose;
  using std::linalg::transposed;

  // [linalg.conjtransposed], conjugated transpose in-place transformation
  using std::linalg::conjugate_transposed;

  // [linalg.algs.blas1], BLAS 1 algorithms
  using std::linalg::add;
  using std::linalg::copy;
  using std::linalg::scale;
  using std::linalg::swap_elements;

  using std::linalg::dot;
  using std::linalg::dotc;

  using std::linalg::sum_of_squares_result;
  using std::linalg::vector_sum_of_squares;

  using std::linalg::vector_two_norm;

  using std::linalg::vector_abs_sum;

  using std::linalg::vector_idx_abs_max;

  using std::linalg::matrix_frob_norm;
  using std::linalg::matrix_inf_norm;
  using std::linalg::matrix_one_norm;

  using std::linalg::apply_givens_rotation;
  using std::linalg::setup_givens_rotation;
  using std::linalg::setup_givens_rotation_result;

  // [linalg.algs.blas2], BLAS 2 algorithms
  using std::linalg::matrix_vector_product;

  using std::linalg::symmetric_matrix_vector_product;

  using std::linalg::hermitian_matrix_vector_product;

  using std::linalg::triangular_matrix_vector_product;

  using std::linalg::triangular_matrix_vector_solve;

  using std::linalg::matrix_rank_1_update;
  using std::linalg::matrix_rank_1_update_c;

  using std::linalg::hermitian_matrix_rank_1_update;
  using std::linalg::symmetric_matrix_rank_1_update;

  using std::linalg::hermitian_matrix_rank_2_update;
  using std::linalg::symmetric_matrix_rank_2_update;

  // [linalg.algs.blas3], BLAS 3 algorithms
  using std::linalg::matrix_product;

  using std::linalg::symmetric_matrix_product;

  using std::linalg::hermitian_matrix_product;

  using std::linalg::triangular_matrix_product;

  using std::linalg::triangular_matrix_left_product;
  using std::linalg::triangular_matrix_right_product;

  using std::linalg::symmetric_matrix_rank_k_update;

  using std::linalg::hermitian_matrix_rank_k_update;

  using std::linalg::symmetric_matrix_rank_2k_update;

  using std::linalg::hermitian_matrix_rank_2k_update;

  using std::linalg::triangular_matrix_matrix_left_solve;
  using std::linalg::triangular_matrix_matrix_right_solve;
#endif // _LIBCPP_STD_VER >= 26
} // namespace std::linalg
