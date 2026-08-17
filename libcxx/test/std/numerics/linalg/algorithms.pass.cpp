//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <linalg>

#include <linalg>

#include <cmath>
#include <complex>
#include <type_traits>

namespace adl_test {

struct complex {
  int re;
  int im;
};

constexpr int abs(complex value) { return value.re < 0 ? -value.re : value.re; }
constexpr int real(complex value) { return value.re; }
constexpr int imag(complex value) { return value.im; }

} // namespace adl_test

constexpr bool test() {
  int input_data[]  = {1, 2, 3, 4};
  int copied_data[] = {0, 0, 0, 0};
  std::mdspan input(input_data, 2, 2);
  std::mdspan copied(copied_data, 2, 2);
  std::linalg::copy(input, copied);
  if (copied[0, 0] != 1 || copied[1, 1] != 4)
    return false;
  std::linalg::scale(3, copied);
  if (copied[0, 1] != 6 || copied[1, 0] != 9)
    return false;

  int lhs_data[] = {1, 2, 2};
  int rhs_data[] = {4, 5, 6};
  int sum_data[] = {0, 0, 0};
  std::mdspan lhs(lhs_data, 3);
  std::mdspan rhs(rhs_data, 3);
  std::mdspan sum(sum_data, 3);
  std::linalg::add(lhs, rhs, sum);
  if (sum[0] != 5 || sum[1] != 7 || sum[2] != 8 || std::linalg::dot(lhs, rhs) != 26 ||
       std::linalg::dot(lhs, rhs, 10) != 36 || std::linalg::vector_two_norm(lhs) != 3 ||
       std::linalg::vector_two_norm(lhs, 4) != 5 || std::linalg::vector_abs_sum(lhs) != 5 ||
       std::linalg::vector_abs_sum(lhs, 10) != 15 || std::linalg::vector_idx_abs_max(lhs) != 1)
    return false;

  auto sum_of_squares = std::linalg::vector_sum_of_squares(
      lhs, std::linalg::sum_of_squares_result<double>{2.0, 3.0});
  if (sum_of_squares.scaling_factor != 2.0 || sum_of_squares.scaled_sum_of_squares != 5.25)
    return false;

  std::complex<double> complex_lhs_data[] = {{1.0, 2.0}, {3.0, -1.0}};
  std::complex<double> complex_rhs_data[] = {{2.0, 1.0}, {-1.0, 4.0}};
  std::mdspan complex_lhs(complex_lhs_data, 2);
  std::mdspan complex_rhs(complex_rhs_data, 2);
  if (std::linalg::dotc(complex_lhs, complex_rhs) != std::complex<double>(-3.0, 8.0) ||
       std::linalg::vector_abs_sum(complex_lhs) != 7.0)
    return false;

  adl_test::complex adl_data[] = {{-3, 4}, {1, -2}};
  std::mdspan adl_vector(adl_data, 2);
  // adl_test::abs returns int, so T is decltype(a * a) == int and the norm is
  // truncated to int per [linalg.algs.blas1.nrm2]; sqrt(10) == 3.16... -> 3.
  static_assert(std::is_same_v<decltype(std::linalg::vector_two_norm(adl_vector)), int>);
  if (std::linalg::vector_two_norm(adl_vector) != 3 || std::linalg::vector_abs_sum(adl_vector) != 10 ||
      std::linalg::vector_idx_abs_max(adl_vector) != 0)
    return false;

  double givens_x_data[] = {3.0, 0.0};
  double givens_y_data[] = {4.0, 5.0};
  std::mdspan givens_x(givens_x_data, 2);
  std::mdspan givens_y(givens_y_data, 2);
  auto rotation = std::linalg::setup_givens_rotation(3.0, 4.0);
  if (rotation.c != 0.6 || rotation.s != 0.8 || rotation.r != 5.0)
    return false;
  std::linalg::apply_givens_rotation(givens_x, givens_y, rotation.c, rotation.s);
  if (std::abs(givens_x[0] - 5.0) > 1e-12 || std::abs(givens_y[0]) > 1e-12)
    return false;

  int swap_data[] = {7, 8, 9};
  std::mdspan swap_view(swap_data, 3);
  std::linalg::swap_elements(lhs, swap_view);
  if (lhs[0] != 7 || swap_view[2] != 2)
    return false;

  int matrix_data[] = {1, 2, 3, 4, 5, 6};
  int vector_data[] = {2, 1, -1};
  int result_data[] = {0, 0};
  std::mdspan matrix(matrix_data, 2, 3);
  // The norms return Scalar, so an int matrix yields int: sqrt(91) == 9.53...
  // -> 9, and with init 2, sqrt(4 + 91) == 9.74... -> 9.
  static_assert(std::is_same_v<decltype(std::linalg::matrix_frob_norm(matrix)), int>);
  static_assert(std::is_same_v<decltype(std::linalg::matrix_frob_norm(matrix, 2)), int>);
  if (std::linalg::matrix_frob_norm(matrix) != 9 || std::linalg::matrix_frob_norm(matrix, 2) != 9 ||
      std::linalg::matrix_one_norm(matrix) != 9 || std::linalg::matrix_one_norm(matrix, 10) != 19 ||
      std::linalg::matrix_inf_norm(matrix) != 15 || std::linalg::matrix_inf_norm(matrix, 10) != 25)
    return false;

  // A floating-point matrix keeps full precision through the same code path.
  double real_data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  std::mdspan real_matrix(real_data, 2, 3);
  static_assert(std::is_same_v<decltype(std::linalg::matrix_frob_norm(real_matrix)), double>);
  if (std::abs(std::linalg::matrix_frob_norm(real_matrix) - std::sqrt(91.0)) > 1e-12)
    return false;
  std::mdspan vector(vector_data, 3);
  std::mdspan result(result_data, 2);
  std::linalg::matrix_vector_product(matrix, vector, result);
  if (result[0] != 1 || result[1] != 7)
    return false;

  int add_data[] = {10, 20};
  std::mdspan add(add_data, 2);
  std::linalg::matrix_vector_product(matrix, vector, add, result);
  if (result[0] != 11 || result[1] != 27)
    return false;

  int right_data[]   = {1, 2, 3, 4, 5, 6};
  int product_data[] = {0, 0, 0, 0};
  std::mdspan right(right_data, 3, 2);
  std::mdspan product(product_data, 2, 2);
  std::linalg::matrix_product(matrix, right, product);
  if (product[0, 0] != 22 || product[0, 1] != 28 || product[1, 0] != 49 || product[1, 1] != 64)
    return false;

  int triangular_data[] = {2, 1, 0, 3};
  int rhs_data_solve[]  = {5, 6};
  int solution_data[]   = {0, 0};
  std::mdspan triangular(triangular_data, 2, 2);
  std::mdspan solve_rhs(rhs_data_solve, 2);
  std::mdspan solution(solution_data, 2);
  std::linalg::triangular_matrix_vector_solve(
      triangular, std::linalg::lower_triangle, std::linalg::explicit_diagonal, solve_rhs, solution);
  if (solution[0] != 2 || solution[1] != 2)
    return false;

  return true;
}

int main(int, char**) { return test() ? 0 : 1; }
