//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// [linalg.algs.blas3.trmm]: triangular_matrix_left_product computes C = A*C and
// triangular_matrix_right_product computes C = C*A, both with C as an
// InOutMat. Because each output element is read while later elements are still
// needed, the traversal order matters: for the left product an upper triangle
// must sweep rows forwards and a lower triangle backwards, and the right
// product mirrors that on columns. A wrong order silently produces a matrix
// built partly from already-overwritten entries, so each in-place result is
// checked against the out-of-place overload.

#include <linalg>

#include <cstddef>

inline constexpr size_t N = 4;

// A is triangular; the entries outside the triangle are never read.
constexpr int a_init[N * N] = {
    2, 3, 4, 5, //
    6, 7, 8, 9, //
    1, 2, 3, 4, //
    5, 6, 7, 8};

constexpr int c_init[N * N] = {
    1, 2, 3, 4,     //
    5, 6, 7, 8,     //
    9, 10, 11, 12,  //
    13, 14, 15, 16};

template <class Triangle, class Diagonal>
constexpr bool check_left(Triangle t, Diagonal d) {
  int a_data[N * N]{};
  int inplace[N * N]{};
  int operand[N * N]{};
  int expected[N * N]{};
  for (size_t i = 0; i != N * N; ++i) {
    a_data[i]  = a_init[i];
    inplace[i] = c_init[i];
    operand[i] = c_init[i];
  }

  std::mdspan a(a_data, N, N);
  std::mdspan ip(inplace, N, N);
  std::mdspan op(operand, N, N);
  std::mdspan ex(expected, N, N);

  // Reference: out-of-place C = A*B with B a pristine copy.
  std::linalg::triangular_matrix_product(a, t, d, op, ex);
  // In-place: C = A*C.
  std::linalg::triangular_matrix_left_product(a, t, d, ip);

  for (size_t i = 0; i != N; ++i)
    for (size_t j = 0; j != N; ++j)
      if (ip[i, j] != ex[i, j])
        return false;
  return true;
}

template <class Triangle, class Diagonal>
constexpr bool check_right(Triangle t, Diagonal d) {
  int a_data[N * N]{};
  int inplace[N * N]{};
  int operand[N * N]{};
  int expected[N * N]{};
  for (size_t i = 0; i != N * N; ++i) {
    a_data[i]  = a_init[i];
    inplace[i] = c_init[i];
    operand[i] = c_init[i];
  }

  std::mdspan a(a_data, N, N);
  std::mdspan ip(inplace, N, N);
  std::mdspan op(operand, N, N);
  std::mdspan ex(expected, N, N);

  // Reference: out-of-place C = B*A (the overload taking the triangular
  // matrix on the right).
  std::linalg::triangular_matrix_product(op, a, t, d, ex);
  // In-place: C = C*A.
  std::linalg::triangular_matrix_right_product(a, t, d, ip);

  for (size_t i = 0; i != N; ++i)
    for (size_t j = 0; j != N; ++j)
      if (ip[i, j] != ex[i, j])
        return false;
  return true;
}

constexpr bool test() {
  return check_left(std::linalg::upper_triangle, std::linalg::explicit_diagonal) &&
         check_left(std::linalg::lower_triangle, std::linalg::explicit_diagonal) &&
         check_left(std::linalg::upper_triangle, std::linalg::implicit_unit_diagonal) &&
         check_left(std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal) &&
         check_right(std::linalg::upper_triangle, std::linalg::explicit_diagonal) &&
         check_right(std::linalg::lower_triangle, std::linalg::explicit_diagonal) &&
         check_right(std::linalg::upper_triangle, std::linalg::implicit_unit_diagonal) &&
         check_right(std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal);
}

int main(int, char**) {
  static_assert(test());
  return test() ? 0 : 1;
}
