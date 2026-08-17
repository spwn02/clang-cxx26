//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// Every symmetric_matrix_rank_k_update overload in the synopsis takes a
// Scalar alpha; there is no alpha-free form. Calling one without alpha must
// not compile.

#include <linalg>

void f() {
  int matrix_data[] = {1, 2, 3, 4};
  int add_data[]    = {1, 2, 3, 4};
  int result_data[] = {0, 0, 0, 0};

  std::mdspan matrix(matrix_data, 2, 2);
  std::mdspan add(add_data, 2, 2);
  std::mdspan result(result_data, 2, 2);

  std::linalg::symmetric_matrix_rank_k_update(matrix, add, result, std::linalg::upper_triangle);
}
