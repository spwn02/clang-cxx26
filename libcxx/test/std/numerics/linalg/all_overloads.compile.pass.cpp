//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
// ADDITIONAL_COMPILE_FLAGS: -D_LIBCPP_ENABLE_EXPERIMENTAL

// <linalg>

// Instantiates every algorithm declaration of [linalg.syn] (sequential and execution-policy overloads) once, so a
// declaration that does not forward to a working implementation is diagnosed. Generated from the synopsis; the calls
// are never executed.

#include <complex>
#include <execution>
#include <functional>
#include <linalg>
#include <mdspan>

using Vec = std::mdspan<double, std::dextents<size_t, 1>>;
using Mat = std::mdspan<double, std::dextents<size_t, 2>>;
using CVec = std::mdspan<std::complex<double>, std::dextents<size_t, 1>>;

// The five-argument out-of-place triangular solves are ambiguous with the in-place overloads taking a BinaryDivideOp:
// neither BinaryDivideOp nor the in-place vector/matrix is constrained apart from the concepts, which do not subsume
// one another ([linalg.syn], an LWG candidate).
template <class... Args>
concept can_solve_vector = requires(Args... args) { std::linalg::triangular_matrix_vector_solve(args...); };
template <class... Args>
concept can_solve_left = requires(Args... args) { std::linalg::triangular_matrix_matrix_left_solve(args...); };
template <class... Args>
concept can_solve_right = requires(Args... args) { std::linalg::triangular_matrix_matrix_right_solve(args...); };
using Tri  = std::linalg::upper_triangle_t;
using Diag = std::linalg::explicit_diagonal_t;
using Seq  = std::execution::sequenced_policy;

void instantiate_all(Vec v, CVec cv, Mat m, Tri tri, Diag diag) {
  (void)v; (void)cv; (void)m; (void)tri; (void)diag;
  // apply_givens_rotation
  std::linalg::apply_givens_rotation(v, v, 0.0, 0.0);
  // apply_givens_rotation
  std::linalg::apply_givens_rotation(std::execution::seq, v, v, 0.0, 0.0);
  // apply_givens_rotation
  std::linalg::apply_givens_rotation(cv, cv, 0.0, std::complex<double>{});
  // apply_givens_rotation
  std::linalg::apply_givens_rotation(std::execution::seq, cv, cv, 0.0, std::complex<double>{});
  // swap_elements
  std::linalg::swap_elements(v, v);
  // swap_elements
  std::linalg::swap_elements(std::execution::seq, v, v);
  // scale
  std::linalg::scale(0.0, v);
  // scale
  std::linalg::scale(std::execution::seq, 0.0, v);
  // copy
  std::linalg::copy(v, v);
  // copy
  std::linalg::copy(std::execution::seq, v, v);
  // add
  std::linalg::add(v, v, v);
  // add
  std::linalg::add(std::execution::seq, v, v, v);
  // dot
  std::linalg::dot(v, v, 0.0);
  // dot
  std::linalg::dot(std::execution::seq, v, v, 0.0);
  // dot
  std::linalg::dot(v, v);
  // dot
  std::linalg::dot(std::execution::seq, v, v);
  // dotc
  std::linalg::dotc(v, v, 0.0);
  // dotc
  std::linalg::dotc(std::execution::seq, v, v, 0.0);
  // dotc
  std::linalg::dotc(v, v);
  // dotc
  std::linalg::dotc(std::execution::seq, v, v);
  // vector_two_norm
  std::linalg::vector_two_norm(v, 0.0);
  // vector_two_norm
  std::linalg::vector_two_norm(std::execution::seq, v, 0.0);
  // vector_two_norm
  std::linalg::vector_two_norm(v);
  // vector_two_norm
  std::linalg::vector_two_norm(std::execution::seq, v);
  // vector_abs_sum
  std::linalg::vector_abs_sum(v, 0.0);
  // vector_abs_sum
  std::linalg::vector_abs_sum(std::execution::seq, v, 0.0);
  // vector_abs_sum
  std::linalg::vector_abs_sum(v);
  // vector_abs_sum
  std::linalg::vector_abs_sum(std::execution::seq, v);
  // vector_idx_abs_max
  std::linalg::vector_idx_abs_max(v);
  // vector_idx_abs_max
  std::linalg::vector_idx_abs_max(std::execution::seq, v);
  // matrix_frob_norm
  std::linalg::matrix_frob_norm(m, 0.0);
  // matrix_frob_norm
  std::linalg::matrix_frob_norm(std::execution::seq, m, 0.0);
  // matrix_frob_norm
  std::linalg::matrix_frob_norm(m);
  // matrix_frob_norm
  std::linalg::matrix_frob_norm(std::execution::seq, m);
  // matrix_one_norm
  std::linalg::matrix_one_norm(m, 0.0);
  // matrix_one_norm
  std::linalg::matrix_one_norm(std::execution::seq, m, 0.0);
  // matrix_one_norm
  std::linalg::matrix_one_norm(m);
  // matrix_one_norm
  std::linalg::matrix_one_norm(std::execution::seq, m);
  // matrix_inf_norm
  std::linalg::matrix_inf_norm(m, 0.0);
  // matrix_inf_norm
  std::linalg::matrix_inf_norm(std::execution::seq, m, 0.0);
  // matrix_inf_norm
  std::linalg::matrix_inf_norm(m);
  // matrix_inf_norm
  std::linalg::matrix_inf_norm(std::execution::seq, m);
  // matrix_vector_product
  std::linalg::matrix_vector_product(m, v, v);
  // matrix_vector_product
  std::linalg::matrix_vector_product(std::execution::seq, m, v, v);
  // matrix_vector_product
  std::linalg::matrix_vector_product(m, v, v, v);
  // matrix_vector_product
  std::linalg::matrix_vector_product(std::execution::seq, m, v, v, v);
  // symmetric_matrix_vector_product
  std::linalg::symmetric_matrix_vector_product(m, tri, v, v);
  // symmetric_matrix_vector_product
  std::linalg::symmetric_matrix_vector_product(std::execution::seq, m, tri, v, v);
  // symmetric_matrix_vector_product
  std::linalg::symmetric_matrix_vector_product(m, tri, v, v, v);
  // symmetric_matrix_vector_product
  std::linalg::symmetric_matrix_vector_product(std::execution::seq, m, tri, v, v, v);
  // hermitian_matrix_vector_product
  std::linalg::hermitian_matrix_vector_product(m, tri, v, v);
  // hermitian_matrix_vector_product
  std::linalg::hermitian_matrix_vector_product(std::execution::seq, m, tri, v, v);
  // hermitian_matrix_vector_product
  std::linalg::hermitian_matrix_vector_product(m, tri, v, v, v);
  // hermitian_matrix_vector_product
  std::linalg::hermitian_matrix_vector_product(std::execution::seq, m, tri, v, v, v);
  // triangular_matrix_vector_product
  std::linalg::triangular_matrix_vector_product(m, tri, diag, v, v);
  // triangular_matrix_vector_product
  std::linalg::triangular_matrix_vector_product(std::execution::seq, m, tri, diag, v, v);
  // triangular_matrix_vector_product
  std::linalg::triangular_matrix_vector_product(m, tri, diag, v);
  // triangular_matrix_vector_product
  std::linalg::triangular_matrix_vector_product(std::execution::seq, m, tri, diag, v);
  // triangular_matrix_vector_product
  std::linalg::triangular_matrix_vector_product(m, tri, diag, v, v, v);
  // triangular_matrix_vector_product
  std::linalg::triangular_matrix_vector_product(std::execution::seq, m, tri, diag, v, v, v);
  // triangular_matrix_vector_solve
  std::linalg::triangular_matrix_vector_solve(m, tri, diag, v, v, std::divides<void>{});
  // triangular_matrix_vector_solve
  std::linalg::triangular_matrix_vector_solve(std::execution::seq, m, tri, diag, v, v, std::divides<void>{});
  // triangular_matrix_vector_solve
  std::linalg::triangular_matrix_vector_solve(m, tri, diag, v, std::divides<void>{});
  // triangular_matrix_vector_solve
  std::linalg::triangular_matrix_vector_solve(std::execution::seq, m, tri, diag, v, std::divides<void>{});
  // matrix_rank_1_update
  std::linalg::matrix_rank_1_update(v, v, m);
  // matrix_rank_1_update
  std::linalg::matrix_rank_1_update(std::execution::seq, v, v, m);
  // matrix_rank_1_update_c
  std::linalg::matrix_rank_1_update_c(v, v, m);
  // matrix_rank_1_update_c
  std::linalg::matrix_rank_1_update_c(std::execution::seq, v, v, m);
  // matrix_rank_1_update
  std::linalg::matrix_rank_1_update(v, v, m, m);
  // matrix_rank_1_update
  std::linalg::matrix_rank_1_update(std::execution::seq, v, v, m, m);
  // matrix_rank_1_update_c
  std::linalg::matrix_rank_1_update_c(v, v, m, m);
  // matrix_rank_1_update_c
  std::linalg::matrix_rank_1_update_c(std::execution::seq, v, v, m, m);
  // symmetric_matrix_rank_1_update
  std::linalg::symmetric_matrix_rank_1_update(0.0, v, m, tri);
  // symmetric_matrix_rank_1_update
  std::linalg::symmetric_matrix_rank_1_update(std::execution::seq, 0.0, v, m, tri);
  // hermitian_matrix_rank_1_update
  std::linalg::hermitian_matrix_rank_1_update(0.0, v, m, tri);
  // hermitian_matrix_rank_1_update
  std::linalg::hermitian_matrix_rank_1_update(std::execution::seq, 0.0, v, m, tri);
  // symmetric_matrix_rank_1_update
  std::linalg::symmetric_matrix_rank_1_update(0.0, v, m, m, tri);
  // symmetric_matrix_rank_1_update
  std::linalg::symmetric_matrix_rank_1_update(std::execution::seq, 0.0, v, m, m, tri);
  // hermitian_matrix_rank_1_update
  std::linalg::hermitian_matrix_rank_1_update(0.0, v, m, m, tri);
  // hermitian_matrix_rank_1_update
  std::linalg::hermitian_matrix_rank_1_update(std::execution::seq, 0.0, v, m, m, tri);
  // symmetric_matrix_rank_2_update
  std::linalg::symmetric_matrix_rank_2_update(v, v, m, tri);
  // symmetric_matrix_rank_2_update
  std::linalg::symmetric_matrix_rank_2_update(std::execution::seq, v, v, m, tri);
  // hermitian_matrix_rank_2_update
  std::linalg::hermitian_matrix_rank_2_update(v, v, m, tri);
  // hermitian_matrix_rank_2_update
  std::linalg::hermitian_matrix_rank_2_update(std::execution::seq, v, v, m, tri);
  // symmetric_matrix_rank_2_update
  std::linalg::symmetric_matrix_rank_2_update(v, v, m, m, tri);
  // symmetric_matrix_rank_2_update
  std::linalg::symmetric_matrix_rank_2_update(std::execution::seq, v, v, m, m, tri);
  // hermitian_matrix_rank_2_update
  std::linalg::hermitian_matrix_rank_2_update(v, v, m, m, tri);
  // hermitian_matrix_rank_2_update
  std::linalg::hermitian_matrix_rank_2_update(std::execution::seq, v, v, m, m, tri);
  // matrix_product
  std::linalg::matrix_product(m, m, m);
  // matrix_product
  std::linalg::matrix_product(std::execution::seq, m, m, m);
  // matrix_product
  std::linalg::matrix_product(m, m, m, m);
  // matrix_product
  std::linalg::matrix_product(std::execution::seq, m, m, m, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(m, tri, m, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(std::execution::seq, m, tri, m, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(m, tri, m, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(std::execution::seq, m, tri, m, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(m, tri, diag, m, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(std::execution::seq, m, tri, diag, m, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(m, m, tri, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(std::execution::seq, m, m, tri, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(m, m, tri, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(std::execution::seq, m, m, tri, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(m, m, tri, diag, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(std::execution::seq, m, m, tri, diag, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(m, tri, m, m, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(std::execution::seq, m, tri, m, m, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(m, tri, m, m, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(std::execution::seq, m, tri, m, m, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(m, tri, diag, m, m, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(std::execution::seq, m, tri, diag, m, m, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(m, m, tri, m, m);
  // symmetric_matrix_product
  std::linalg::symmetric_matrix_product(std::execution::seq, m, m, tri, m, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(m, m, tri, m, m);
  // hermitian_matrix_product
  std::linalg::hermitian_matrix_product(std::execution::seq, m, m, tri, m, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(m, m, tri, diag, m, m);
  // triangular_matrix_product
  std::linalg::triangular_matrix_product(std::execution::seq, m, m, tri, diag, m, m);
  // triangular_matrix_left_product
  std::linalg::triangular_matrix_left_product(m, tri, diag, m);
  // triangular_matrix_left_product
  std::linalg::triangular_matrix_left_product(std::execution::seq, m, tri, diag, m);
  // triangular_matrix_right_product
  std::linalg::triangular_matrix_right_product(m, tri, diag, m);
  // triangular_matrix_right_product
  std::linalg::triangular_matrix_right_product(std::execution::seq, m, tri, diag, m);
  // symmetric_matrix_rank_k_update
  std::linalg::symmetric_matrix_rank_k_update(0.0, m, m, tri);
  // symmetric_matrix_rank_k_update
  std::linalg::symmetric_matrix_rank_k_update(std::execution::seq, 0.0, m, m, tri);
  // hermitian_matrix_rank_k_update
  std::linalg::hermitian_matrix_rank_k_update(0.0, m, m, tri);
  // hermitian_matrix_rank_k_update
  std::linalg::hermitian_matrix_rank_k_update(std::execution::seq, 0.0, m, m, tri);
  // symmetric_matrix_rank_k_update
  std::linalg::symmetric_matrix_rank_k_update(0.0, m, m, m, tri);
  // symmetric_matrix_rank_k_update
  std::linalg::symmetric_matrix_rank_k_update(std::execution::seq, 0.0, m, m, m, tri);
  // hermitian_matrix_rank_k_update
  std::linalg::hermitian_matrix_rank_k_update(0.0, m, m, m, tri);
  // hermitian_matrix_rank_k_update
  std::linalg::hermitian_matrix_rank_k_update(std::execution::seq, 0.0, m, m, m, tri);
  // symmetric_matrix_rank_2k_update
  std::linalg::symmetric_matrix_rank_2k_update(m, m, m, tri);
  // symmetric_matrix_rank_2k_update
  std::linalg::symmetric_matrix_rank_2k_update(std::execution::seq, m, m, m, tri);
  // hermitian_matrix_rank_2k_update
  std::linalg::hermitian_matrix_rank_2k_update(m, m, m, tri);
  // hermitian_matrix_rank_2k_update
  std::linalg::hermitian_matrix_rank_2k_update(std::execution::seq, m, m, m, tri);
  // symmetric_matrix_rank_2k_update
  std::linalg::symmetric_matrix_rank_2k_update(m, m, m, m, tri);
  // symmetric_matrix_rank_2k_update
  std::linalg::symmetric_matrix_rank_2k_update(std::execution::seq, m, m, m, m, tri);
  // hermitian_matrix_rank_2k_update
  std::linalg::hermitian_matrix_rank_2k_update(m, m, m, m, tri);
  // hermitian_matrix_rank_2k_update
  std::linalg::hermitian_matrix_rank_2k_update(std::execution::seq, m, m, m, m, tri);
  // triangular_matrix_matrix_left_solve
  std::linalg::triangular_matrix_matrix_left_solve(m, tri, diag, m, m, std::divides<void>{});
  // triangular_matrix_matrix_left_solve
  std::linalg::triangular_matrix_matrix_left_solve(std::execution::seq, m, tri, diag, m, m, std::divides<void>{});
  // triangular_matrix_matrix_right_solve
  std::linalg::triangular_matrix_matrix_right_solve(m, tri, diag, m, m, std::divides<void>{});
  // triangular_matrix_matrix_right_solve
  std::linalg::triangular_matrix_matrix_right_solve(std::execution::seq, m, tri, diag, m, m, std::divides<void>{});
  // triangular_matrix_matrix_left_solve
  std::linalg::triangular_matrix_matrix_left_solve(m, tri, diag, m, std::divides<void>{});
  // triangular_matrix_matrix_left_solve
  std::linalg::triangular_matrix_matrix_left_solve(std::execution::seq, m, tri, diag, m, std::divides<void>{});
  // triangular_matrix_matrix_right_solve
  std::linalg::triangular_matrix_matrix_right_solve(m, tri, diag, m, std::divides<void>{});
  // triangular_matrix_matrix_right_solve
  std::linalg::triangular_matrix_matrix_right_solve(std::execution::seq, m, tri, diag, m, std::divides<void>{});
}

// as specified, calling an out-of-place solve without a divide operation is ambiguous
static_assert(!can_solve_vector<Mat, Tri, Diag, Vec, Vec>);
static_assert(!can_solve_vector<Seq, Mat, Tri, Diag, Vec, Vec>);
static_assert(!can_solve_vector<Mat, Tri, Diag, Vec, Vec>);
static_assert(!can_solve_vector<Seq, Mat, Tri, Diag, Vec, Vec>);
static_assert(!can_solve_left<Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_left<Seq, Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_right<Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_right<Seq, Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_left<Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_left<Seq, Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_right<Mat, Tri, Diag, Mat, Mat>);
static_assert(!can_solve_right<Seq, Mat, Tri, Diag, Mat, Mat>);
