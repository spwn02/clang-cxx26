//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// The updating overloads must reject a read-only output just as the
// overwriting ones do.

#include <linalg>

void f() {
  int m_data[]       = {2, 1, 1, 3};
  int o_data[]       = {1, 2, 3, 4};
  int e_data[]       = {0, 0, 0, 0};
  const int c_data[] = {0, 0, 0, 0};

  std::mdspan m(m_data, 2, 2);
  std::mdspan o(o_data, 2, 2);
  std::mdspan e(e_data, 2, 2);
  std::mdspan c(c_data, 2, 2);

  std::linalg::symmetric_matrix_product(m, std::linalg::upper_triangle, o, e, c);
}
