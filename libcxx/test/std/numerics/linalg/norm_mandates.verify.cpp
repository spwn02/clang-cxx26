//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// [linalg.algs.blas1.nrm2], [linalg.algs.blas1.matfrobnorm]: the value type and Scalar are floating-point types or
// specializations of complex.

#include <linalg>
#include <mdspan>

// expected-note@*:* 0+ {{in instantiation of}}

void integral_value_type() {
  int v[2] = {3, 4};
  std::mdspan<int, std::extents<int, 2>> vec(v);
  // expected-error@*:* {{linalg::vector_two_norm requires a floating-point or complex value_type and Scalar}}
  (void)std::linalg::vector_two_norm(vec, 0.0);
  int m[4] = {1, 2, 3, 4};
  std::mdspan<int, std::extents<int, 2, 2>> mat(m);
  // expected-error@*:* {{linalg::matrix_frob_norm requires a floating-point or complex value_type and Scalar}}
  (void)std::linalg::matrix_frob_norm(mat, 0.0);
}

void integral_scalar() {
  double v[2] = {3, 4};
  std::mdspan<double, std::extents<int, 2>> vec(v);
  // expected-error@*:* {{linalg::vector_two_norm requires a floating-point or complex value_type and Scalar}}
  (void)std::linalg::vector_two_norm(vec, 0);
  double m[4] = {1, 2, 3, 4};
  std::mdspan<double, std::extents<int, 2, 2>> mat(m);
  // expected-error@*:* {{linalg::matrix_frob_norm requires a floating-point or complex value_type and Scalar}}
  (void)std::linalg::matrix_frob_norm(mat, 0);
}
