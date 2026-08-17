//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// A statically mismatched inner dimension must be a compile error in the
// updating overloads, not a runtime out-of-bounds walk.

#include <linalg>

void f() {
  int m_data[] = {2, 1, 1, 3};
  int o_data[] = {1, 2, 3, 4, 5, 6};
  int e_data[] = {0, 0, 0, 0};
  int c_data[] = {0, 0, 0, 0};

  // A is 2x2 but B is 3x2, so A's inner extent does not match B's outer one.
  std::mdspan<int, std::extents<size_t, 2, 2>> m(m_data);
  std::mdspan<int, std::extents<size_t, 3, 2>> o(o_data);
  std::mdspan<int, std::extents<size_t, 2, 2>> e(e_data);
  std::mdspan<int, std::extents<size_t, 2, 2>> c(c_data);

  std::linalg::symmetric_matrix_product(m, std::linalg::upper_triangle, o, e, c);
}
