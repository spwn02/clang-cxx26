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

// Every algorithm family has an execution-policy overload. This implementation
// satisfies them with the permitted serial fallback, so each policy overload
// must agree with its sequential counterpart.

#include <linalg>

#include <cmath>
#include <complex>
#include <execution>

namespace ex = std::execution;

static bool test_blas1() {
  int input_data[]  = {1, 2, 3, 4};
  int copied_data[] = {0, 0, 0, 0};
  std::mdspan input(input_data, 2, 2);
  std::mdspan copied(copied_data, 2, 2);
  std::linalg::copy(ex::seq, input, copied);
  if (copied[0, 0] != 1 || copied[1, 1] != 4)
    return false;

  std::linalg::scale(ex::seq, 3, copied);
  if (copied[0, 1] != 6 || copied[1, 0] != 9)
    return false;

  int lhs_data[] = {1, 2, 2};
  int rhs_data[] = {4, 5, 6};
  int sum_data[] = {0, 0, 0};
  std::mdspan lhs(lhs_data, 3);
  std::mdspan rhs(rhs_data, 3);
  std::mdspan sum(sum_data, 3);
  std::linalg::add(ex::seq, lhs, rhs, sum);
  if (sum[0] != 5 || sum[1] != 7 || sum[2] != 8)
    return false;

  if (std::linalg::dot(ex::seq, lhs, rhs) != 26 || std::linalg::dot(ex::seq, lhs, rhs, 10) != 36 ||
      std::linalg::vector_two_norm(ex::seq, lhs) != 3 || std::linalg::vector_two_norm(ex::seq, lhs, 4) != 5 ||
      std::linalg::vector_abs_sum(ex::seq, lhs) != 5 || std::linalg::vector_abs_sum(ex::seq, lhs, 10) != 15 ||
      std::linalg::vector_idx_abs_max(ex::seq, lhs) != 1)
    return false;

  auto sum_of_squares =
      std::linalg::vector_sum_of_squares(ex::seq, lhs, std::linalg::sum_of_squares_result<double>{2.0, 3.0});
  if (sum_of_squares.scaling_factor != 2.0 || sum_of_squares.scaled_sum_of_squares != 5.25)
    return false;

  std::complex<double> complex_lhs_data[] = {{1.0, 2.0}, {3.0, -1.0}};
  std::complex<double> complex_rhs_data[] = {{2.0, 1.0}, {-1.0, 4.0}};
  std::mdspan complex_lhs(complex_lhs_data, 2);
  std::mdspan complex_rhs(complex_rhs_data, 2);
  if (std::linalg::dotc(ex::seq, complex_lhs, complex_rhs) != std::complex<double>(-3.0, 8.0))
    return false;

  double givens_x_data[] = {3.0, 0.0};
  double givens_y_data[] = {4.0, 5.0};
  std::mdspan givens_x(givens_x_data, 2);
  std::mdspan givens_y(givens_y_data, 2);
  auto rotation = std::linalg::setup_givens_rotation(3.0, 4.0);
  std::linalg::apply_givens_rotation(ex::seq, givens_x, givens_y, rotation.c, rotation.s);
  if (std::abs(givens_x[0] - 5.0) > 1e-12 || std::abs(givens_y[0]) > 1e-12)
    return false;

  int swap_data[] = {7, 8, 9};
  std::mdspan swap_view(swap_data, 3);
  std::linalg::swap_elements(ex::seq, lhs, swap_view);
  if (lhs[0] != 7 || swap_view[2] != 2)
    return false;

  return true;
}

static bool test_norms_and_products() {
  int matrix_data[] = {1, 2, 3, 4, 5, 6};
  int vector_data[] = {2, 1, -1};
  int result_data[] = {0, 0};
  std::mdspan matrix(matrix_data, 2, 3);
  std::mdspan vector(vector_data, 3);
  std::mdspan result(result_data, 2);

  // int matrix -> int norms, matching the sequential overloads.
  if (std::linalg::matrix_frob_norm(ex::seq, matrix) != 9 || std::linalg::matrix_frob_norm(ex::seq, matrix, 2) != 9 ||
      std::linalg::matrix_one_norm(ex::seq, matrix) != 9 || std::linalg::matrix_one_norm(ex::seq, matrix, 10) != 19 ||
      std::linalg::matrix_inf_norm(ex::seq, matrix) != 15 || std::linalg::matrix_inf_norm(ex::seq, matrix, 10) != 25)
    return false;

  std::linalg::matrix_vector_product(ex::seq, matrix, vector, result);
  if (result[0] != 1 || result[1] != 7)
    return false;

  int add_data[] = {10, 20};
  std::mdspan add_view(add_data, 2);
  std::linalg::matrix_vector_product(ex::seq, matrix, vector, add_view, result);
  if (result[0] != 11 || result[1] != 27)
    return false;

  int right_data[]   = {1, 2, 3, 4, 5, 6};
  int product_data[] = {0, 0, 0, 0};
  std::mdspan right(right_data, 3, 2);
  std::mdspan product(product_data, 2, 2);
  std::linalg::matrix_product(ex::seq, matrix, right, product);
  if (product[0, 0] != 22 || product[0, 1] != 28 || product[1, 0] != 49 || product[1, 1] != 64)
    return false;

  int triangular_data[] = {2, 1, 0, 3};
  int rhs_data_solve[]  = {5, 6};
  int solution_data[]   = {0, 0};
  std::mdspan triangular(triangular_data, 2, 2);
  std::mdspan solve_rhs(rhs_data_solve, 2);
  std::mdspan solution(solution_data, 2);
  std::linalg::triangular_matrix_vector_solve(
      ex::seq, triangular, std::linalg::lower_triangle, std::linalg::explicit_diagonal, solve_rhs, solution);
  if (solution[0] != 2 || solution[1] != 2)
    return false;

  return true;
}

// The policy overloads are generic in the policy type, so confirm a second
// policy also selects them rather than only decay-matching ex::seq.
static bool test_other_policies() {
  int lhs_data[] = {1, 2, 2};
  int rhs_data[] = {4, 5, 6};
  int sum_data[] = {0, 0, 0};
  std::mdspan lhs(lhs_data, 3);
  std::mdspan rhs(rhs_data, 3);
  std::mdspan sum(sum_data, 3);

  std::linalg::add(ex::unseq, lhs, rhs, sum);
  if (sum[0] != 5 || sum[2] != 8)
    return false;
  if (std::linalg::dot(ex::par, lhs, rhs) != 26)
    return false;
  if (std::linalg::dot(ex::par_unseq, lhs, rhs) != 26)
    return false;

  return true;
}

// add() accepts rank-0, rank-1 and rank-2 operands, matching copy().
static bool test_add_ranks() {
  int left_data[]   = {1, 2, 3, 4};
  int right_data[]  = {10, 20, 30, 40};
  int output_data[] = {0, 0, 0, 0};
  std::mdspan left(left_data, 2, 2);
  std::mdspan right(right_data, 2, 2);
  std::mdspan output(output_data, 2, 2);
  std::linalg::add(left, right, output);
  if (output[0, 0] != 11 || output[0, 1] != 22 || output[1, 0] != 33 || output[1, 1] != 44)
    return false;

  std::linalg::add(ex::seq, left, right, output);
  if (output[1, 1] != 44)
    return false;

  int scalar_left = 3, scalar_right = 4, scalar_out = 0;
  std::mdspan<int, std::extents<size_t>> sl(&scalar_left);
  std::mdspan<int, std::extents<size_t>> sr(&scalar_right);
  std::mdspan<int, std::extents<size_t>> so(&scalar_out);
  std::linalg::add(sl, sr, so);
  if (scalar_out != 7)
    return false;

  return true;
}

int main(int, char**) {
  return test_blas1() && test_norms_and_products() && test_other_policies() && test_add_ranks() ? 0 : 1;
}
