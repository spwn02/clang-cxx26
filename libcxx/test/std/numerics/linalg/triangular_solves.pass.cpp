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
  int lower_data[]    = {1, 99, 2, 1};
  int rhs_data[]      = {3, 7};
  int solution_data[] = {0, 0};
  std::mdspan lower(lower_data, 2, 2);
  std::mdspan rhs(rhs_data, 2);
  std::mdspan solution(solution_data, 2);
  std::linalg::triangular_matrix_vector_solve(
      lower, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, rhs, solution);
  if (solution[0] != 3 || solution[1] != 1)
    return 1;
  std::linalg::triangular_matrix_vector_solve(
      lower, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, rhs);
  if (rhs[0] != 3 || rhs[1] != 1)
    return 1;

  int upper_data[]         = {2, 1, 99, 3};
  int left_rhs_data[]      = {5, 8, 4, 10};
  int left_solution_data[] = {0, 0, 0, 0};
  std::mdspan upper(upper_data, 2, 2);
  std::mdspan left_rhs(left_rhs_data, 2, 2);
  std::mdspan left_solution(left_solution_data, 2, 2);
  std::linalg::triangular_matrix_matrix_left_solve(
      upper, std::linalg::upper_triangle, std::linalg::explicit_diagonal, left_rhs, left_solution);
  if (left_solution[0, 0] != 2 || left_solution[0, 1] != 2 || left_solution[1, 0] != 1 || left_solution[1, 1] != 3)
    return 1;

  int left_inout_data[] = {5, 8, 4, 10};
  std::mdspan left_inout(left_inout_data, 2, 2);
  std::linalg::triangular_matrix_matrix_left_solve(
      upper, std::linalg::upper_triangle, std::linalg::explicit_diagonal, left_inout);
  if (left_inout[0, 0] != 2 || left_inout[0, 1] != 2 || left_inout[1, 0] != 1 || left_inout[1, 1] != 3)
    return 1;

  int right_rhs_data[]      = {5, 8, 4, 10};
  int right_solution_data[] = {0, 0, 0, 0};
  std::mdspan right_rhs(right_rhs_data, 2, 2);
  std::mdspan right_solution(right_solution_data, 2, 2);
  std::linalg::triangular_matrix_matrix_right_solve(
      lower, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, right_rhs, right_solution);
  if (right_solution[0, 0] != -11 || right_solution[0, 1] != 8 || right_solution[1, 0] != -16 ||
      right_solution[1, 1] != 10)
    return 1;

  int right_inout_data[] = {5, 8, 4, 10};
  std::mdspan right_inout(right_inout_data, 2, 2);
  std::linalg::triangular_matrix_matrix_right_solve(
      lower, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, right_inout);
  if (right_inout[0, 0] != -11 || right_inout[0, 1] != 8 || right_inout[1, 0] != -16 || right_inout[1, 1] != 10)
    return 1;

  int divided_rhs_data[] = {5, 6};
  std::mdspan divided_rhs(divided_rhs_data, 2);
  std::linalg::triangular_matrix_vector_solve(
      upper, std::linalg::upper_triangle, std::linalg::explicit_diagonal, divided_rhs,
      [](int value, int diagonal) { return value / diagonal; });
  if (divided_rhs[0] != 1 || divided_rhs[1] != 2)
    return 1;

  return 0;
}
