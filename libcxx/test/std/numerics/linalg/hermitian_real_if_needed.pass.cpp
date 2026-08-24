//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// [linalg.algs.blas2.symherrank1], [linalg.algs.blas2.rank2],
// [linalg.algs.blas3.rankk], and [linalg.algs.blas3.rank2k] specify:
//  - for hermitian_matrix_rank_1_update and hermitian_matrix_rank_k_update,
//    the scalar factor alpha is used as real-if-needed(alpha), not alpha
//    itself (P3371R5, so the update stays mathematically Hermitian even if
//    alpha has nonzero imaginary part);
//  - for every "updating" (E-taking) hermitian_matrix_rank_{1,2,k,2k}_update
//    overload, a *diagonal* access of E uses real-if-needed(E[i, i]); other
//    accesses of E use E[i, j] unchanged.
// This wasn't exercised by rank_updates.pass.cpp, whose alpha and E diagonal
// entries all already happened to be real. These checks use inputs with a
// genuinely nonzero imaginary part in exactly those two spots, so a
// regression (dropping either real-if-needed) shows up as a nonzero
// imaginary part on a Hermitian matrix's diagonal.

#include <linalg>

#include <complex>

int main(int, char**) {
  using complex = std::complex<int>;

  // hermitian_matrix_rank_1_update, overwriting overload: alpha has nonzero
  // imaginary part; only its real part must be used.
  {
    int x_data[]   = {1, 2};
    std::mdspan x(x_data, 2);
    complex a_data[] = {{}, {}, {}, {}};
    std::mdspan a(a_data, 2, 2);
    std::linalg::hermitian_matrix_rank_1_update(complex(2, 3), x, a, std::linalg::lower_triangle);
    if (a[0, 0] != complex(2, 0) || a[1, 0] != complex(4, 0) || a[1, 1] != complex(8, 0))
      return 1;
  }

  // hermitian_matrix_rank_1_update, updating overload: alpha real, but E's
  // diagonal has nonzero imaginary part; only E's off-diagonal keeps it.
  {
    int x_data[] = {1, 2};
    std::mdspan x(x_data, 2);
    complex e_data[] = {{5, 7}, {}, {4, 1}, {9, -3}};
    std::mdspan e(e_data, 2, 2);
    complex a_data[] = {{}, {}, {}, {}};
    std::mdspan a(a_data, 2, 2);
    std::linalg::hermitian_matrix_rank_1_update(1, x, e, a, std::linalg::lower_triangle);
    if (a[0, 0] != complex(6, 0) || a[1, 0] != complex(6, 1) || a[1, 1] != complex(13, 0))
      return 1;
  }

  // hermitian_matrix_rank_k_update, overwriting overload: alpha has nonzero
  // imaginary part.
  {
    complex m_data[] = {{1, 0}, {0, 1}};
    std::mdspan m(m_data, 2, 1);
    complex c_data[] = {{}, {}, {}, {}};
    std::mdspan c(c_data, 2, 2);
    std::linalg::hermitian_matrix_rank_k_update(complex(2, 5), m, c, std::linalg::lower_triangle);
    if (c[0, 0] != complex(2, 0) || c[1, 0] != complex(0, 2) || c[1, 1] != complex(2, 0))
      return 1;
  }

  // hermitian_matrix_rank_k_update, updating overload: alpha real, E's
  // diagonal has nonzero imaginary part.
  {
    complex m_data[] = {{1, 0}, {0, 1}};
    std::mdspan m(m_data, 2, 1);
    complex e_data[] = {{3, 4}, {}, {2, -1}, {6, -2}};
    std::mdspan e(e_data, 2, 2);
    complex c_data[] = {{}, {}, {}, {}};
    std::mdspan c(c_data, 2, 2);
    std::linalg::hermitian_matrix_rank_k_update(1, m, e, c, std::linalg::lower_triangle);
    if (c[0, 0] != complex(4, 0) || c[1, 0] != complex(2, 0) || c[1, 1] != complex(7, 0))
      return 1;
  }

  // hermitian_matrix_rank_2_update, updating overload (no alpha parameter):
  // E's diagonal has nonzero imaginary part.
  {
    int x_data[] = {1, 2};
    int y_data[] = {3, 4};
    std::mdspan x(x_data, 2);
    std::mdspan y(y_data, 2);
    complex e_data[] = {{5, 9}, {}, {2, -3}, {7, -6}};
    std::mdspan e(e_data, 2, 2);
    complex a_data[] = {{}, {}, {}, {}};
    std::mdspan a(a_data, 2, 2);
    std::linalg::hermitian_matrix_rank_2_update(x, y, e, a, std::linalg::lower_triangle);
    if (a[0, 0] != complex(11, 0) || a[1, 0] != complex(12, -3) || a[1, 1] != complex(23, 0))
      return 1;
  }

  // hermitian_matrix_rank_2k_update, updating overload (no alpha parameter):
  // E's diagonal has nonzero imaginary part.
  {
    complex left_data[]  = {{1, 0}, {0, 1}};
    complex right_data[] = {{2, 0}, {0, 3}};
    std::mdspan left(left_data, 2, 1);
    std::mdspan right(right_data, 2, 1);
    complex e_data[] = {{3, 8}, {}, {1, -2}, {4, -9}};
    std::mdspan e(e_data, 2, 2);
    complex c_data[] = {{}, {}, {}, {}};
    std::mdspan c(c_data, 2, 2);
    std::linalg::hermitian_matrix_rank_2k_update(left, right, e, c, std::linalg::lower_triangle);
    if (c[0, 0] != complex(7, 0) || c[1, 0] != complex(1, 3) || c[1, 1] != complex(10, 0))
      return 1;
  }

  return 0;
}
