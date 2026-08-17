//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// The updating BLAS 3 overloads compute C = E + A*B, and [linalg.algs.blas3.gemm]
// and [linalg.algs.blas3.xxmm] both state "C may alias E". An implementation
// that computes the product into C first and adds E afterwards double-counts in
// that case, so each of these checks the aliased result against the
// non-aliased one.

#include <linalg>

#include <complex>

// C aliasing E is expressed by passing the same mdspan as both the addend and
// the output.
constexpr bool test_matrix_product() {
  int a_data[] = {1, 2, 3, 4};
  int b_data[] = {5, 6, 7, 8};
  std::mdspan a(a_data, 2, 2);
  std::mdspan b(b_data, 2, 2);

  int separate_e[] = {100, 200, 300, 400};
  int separate_c[] = {0, 0, 0, 0};
  std::mdspan se(separate_e, 2, 2);
  std::mdspan sc(separate_c, 2, 2);
  std::linalg::matrix_product(a, b, se, sc);

  int aliased[] = {100, 200, 300, 400};
  std::mdspan al(aliased, 2, 2);
  std::linalg::matrix_product(a, b, al, al);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (al[i, j] != sc[i, j])
        return false;
  return true;
}

constexpr bool test_symmetric_matrix_product() {
  int m_data[] = {2, 1, 1, 3};
  int o_data[] = {1, 2, 3, 4};
  std::mdspan m(m_data, 2, 2);
  std::mdspan o(o_data, 2, 2);

  int separate_e[] = {10, 20, 30, 40};
  int separate_c[] = {0, 0, 0, 0};
  std::mdspan se(separate_e, 2, 2);
  std::mdspan sc(separate_c, 2, 2);
  std::linalg::symmetric_matrix_product(m, std::linalg::upper_triangle, o, se, sc);

  int aliased[] = {10, 20, 30, 40};
  std::mdspan al(aliased, 2, 2);
  std::linalg::symmetric_matrix_product(m, std::linalg::upper_triangle, o, al, al);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (al[i, j] != sc[i, j])
        return false;

  // Pin the absolute values so this is not merely self-consistent: A*B is
  // {{5,8},{10,14}} and E is {{10,20},{30,40}}, so C = E + A*B is
  // {{15,28},{40,54}}. Computing the product first and adding E afterwards
  // would instead yield 2*A*B == {{10,16},{20,28}}.
  if (al[0, 0] != 15 || al[0, 1] != 28 || al[1, 0] != 40 || al[1, 1] != 54)
    return false;

  // Same check for the overload taking the symmetric matrix on the right.
  int sep2[] = {10, 20, 30, 40};
  int out2[] = {0, 0, 0, 0};
  std::mdspan s2(sep2, 2, 2);
  std::mdspan o2(out2, 2, 2);
  std::linalg::symmetric_matrix_product(o, m, std::linalg::upper_triangle, s2, o2);

  int alias2[] = {10, 20, 30, 40};
  std::mdspan a2(alias2, 2, 2);
  std::linalg::symmetric_matrix_product(o, m, std::linalg::upper_triangle, a2, a2);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (a2[i, j] != o2[i, j])
        return false;
  return true;
}

constexpr bool test_hermitian_matrix_product() {
  using C = std::complex<double>;
  C m_data[] = {{2.0, 0.0}, {1.0, 1.0}, {1.0, -1.0}, {3.0, 0.0}};
  C o_data[] = {{1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}};
  std::mdspan m(m_data, 2, 2);
  std::mdspan o(o_data, 2, 2);

  C sep[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
  C out[] = {{}, {}, {}, {}};
  std::mdspan se(sep, 2, 2);
  std::mdspan sc(out, 2, 2);
  std::linalg::hermitian_matrix_product(m, std::linalg::upper_triangle, o, se, sc);

  C ali[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
  std::mdspan al(ali, 2, 2);
  std::linalg::hermitian_matrix_product(m, std::linalg::upper_triangle, o, al, al);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (al[i, j] != sc[i, j])
        return false;

  C sep2[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
  C out2[] = {{}, {}, {}, {}};
  std::mdspan s2(sep2, 2, 2);
  std::mdspan o2(out2, 2, 2);
  std::linalg::hermitian_matrix_product(o, m, std::linalg::upper_triangle, s2, o2);

  C ali2[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
  std::mdspan a2(ali2, 2, 2);
  std::linalg::hermitian_matrix_product(o, m, std::linalg::upper_triangle, a2, a2);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (a2[i, j] != o2[i, j])
        return false;
  return true;
}

constexpr bool test_triangular_matrix_product() {
  int m_data[] = {2, 1, 0, 3};
  int o_data[] = {1, 2, 3, 4};
  std::mdspan m(m_data, 2, 2);
  std::mdspan o(o_data, 2, 2);

  int sep[] = {10, 20, 30, 40};
  int out[] = {0, 0, 0, 0};
  std::mdspan se(sep, 2, 2);
  std::mdspan sc(out, 2, 2);
  std::linalg::triangular_matrix_product(
      m, std::linalg::upper_triangle, std::linalg::explicit_diagonal, o, se, sc);

  int ali[] = {10, 20, 30, 40};
  std::mdspan al(ali, 2, 2);
  std::linalg::triangular_matrix_product(
      m, std::linalg::upper_triangle, std::linalg::explicit_diagonal, o, al, al);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (al[i, j] != sc[i, j])
        return false;

  int sep2[] = {10, 20, 30, 40};
  int out2[] = {0, 0, 0, 0};
  std::mdspan s2(sep2, 2, 2);
  std::mdspan o2(out2, 2, 2);
  std::linalg::triangular_matrix_product(
      o, m, std::linalg::upper_triangle, std::linalg::explicit_diagonal, s2, o2);

  int ali2[] = {10, 20, 30, 40};
  std::mdspan a2(ali2, 2, 2);
  std::linalg::triangular_matrix_product(
      o, m, std::linalg::upper_triangle, std::linalg::explicit_diagonal, a2, a2);

  for (size_t i = 0; i != 2; ++i)
    for (size_t j = 0; j != 2; ++j)
      if (a2[i, j] != o2[i, j])
        return false;
  return true;
}

// [linalg.algs.blas2.gemv] likewise permits z to alias y.
constexpr bool test_matrix_vector_product() {
  int m_data[] = {1, 2, 3, 4};
  int v_data[] = {5, 6};
  std::mdspan m(m_data, 2, 2);
  std::mdspan v(v_data, 2);

  int sep[] = {100, 200};
  int out[] = {0, 0};
  std::mdspan sy(sep, 2);
  std::mdspan sz(out, 2);
  std::linalg::matrix_vector_product(m, v, sy, sz);

  int ali[] = {100, 200};
  std::mdspan al(ali, 2);
  std::linalg::matrix_vector_product(m, v, al, al);

  return al[0] == sz[0] && al[1] == sz[1];
}

// The rewritten updating overloads select their stored/unstored element and
// their loop bounds from the triangle and diagonal tags, so the lower_triangle
// and implicit_unit_diagonal branches need coverage of their own rather than
// only the upper/explicit ones used above.
constexpr bool test_lower_and_implicit_branches() {
  int m_data[] = {2, 1, 1, 3};
  int o_data[] = {1, 2, 3, 4};
  std::mdspan m(m_data, 2, 2);
  std::mdspan o(o_data, 2, 2);

  // symmetric, lower_triangle, both operand orders.
  {
    int sep[] = {10, 20, 30, 40};
    int out[] = {0, 0, 0, 0};
    std::mdspan se(sep, 2, 2);
    std::mdspan sc(out, 2, 2);
    std::linalg::symmetric_matrix_product(m, std::linalg::lower_triangle, o, se, sc);

    int ali[] = {10, 20, 30, 40};
    std::mdspan al(ali, 2, 2);
    std::linalg::symmetric_matrix_product(m, std::linalg::lower_triangle, o, al, al);
    for (size_t i = 0; i != 2; ++i)
      for (size_t j = 0; j != 2; ++j)
        if (al[i, j] != sc[i, j])
          return false;

    int sep2[] = {10, 20, 30, 40};
    int out2[] = {0, 0, 0, 0};
    std::mdspan s2(sep2, 2, 2);
    std::mdspan o2(out2, 2, 2);
    std::linalg::symmetric_matrix_product(o, m, std::linalg::lower_triangle, s2, o2);

    int ali2[] = {10, 20, 30, 40};
    std::mdspan a2(ali2, 2, 2);
    std::linalg::symmetric_matrix_product(o, m, std::linalg::lower_triangle, a2, a2);
    for (size_t i = 0; i != 2; ++i)
      for (size_t j = 0; j != 2; ++j)
        if (a2[i, j] != o2[i, j])
          return false;
  }

  // triangular, lower_triangle with implicit_unit_diagonal, both orders.
  {
    int sep[] = {10, 20, 30, 40};
    int out[] = {0, 0, 0, 0};
    std::mdspan se(sep, 2, 2);
    std::mdspan sc(out, 2, 2);
    std::linalg::triangular_matrix_product(
        m, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, o, se, sc);

    int ali[] = {10, 20, 30, 40};
    std::mdspan al(ali, 2, 2);
    std::linalg::triangular_matrix_product(
        m, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, o, al, al);
    for (size_t i = 0; i != 2; ++i)
      for (size_t j = 0; j != 2; ++j)
        if (al[i, j] != sc[i, j])
          return false;

    int sep2[] = {10, 20, 30, 40};
    int out2[] = {0, 0, 0, 0};
    std::mdspan s2(sep2, 2, 2);
    std::mdspan o2(out2, 2, 2);
    std::linalg::triangular_matrix_product(
        o, m, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, s2, o2);

    int ali2[] = {10, 20, 30, 40};
    std::mdspan a2(ali2, 2, 2);
    std::linalg::triangular_matrix_product(
        o, m, std::linalg::lower_triangle, std::linalg::implicit_unit_diagonal, a2, a2);
    for (size_t i = 0; i != 2; ++i)
      for (size_t j = 0; j != 2; ++j)
        if (a2[i, j] != o2[i, j])
          return false;
  }

  // hermitian, lower_triangle, both operand orders.
  {
    using C = std::complex<double>;
    C hm[]  = {{2.0, 0.0}, {1.0, 1.0}, {1.0, -1.0}, {3.0, 0.0}};
    C ho[]  = {{1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}};
    std::mdspan hmv(hm, 2, 2);
    std::mdspan hov(ho, 2, 2);

    C sep[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
    C out[] = {{}, {}, {}, {}};
    std::mdspan se(sep, 2, 2);
    std::mdspan sc(out, 2, 2);
    std::linalg::hermitian_matrix_product(hmv, std::linalg::lower_triangle, hov, se, sc);

    C ali[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
    std::mdspan al(ali, 2, 2);
    std::linalg::hermitian_matrix_product(hmv, std::linalg::lower_triangle, hov, al, al);
    for (size_t i = 0; i != 2; ++i)
      for (size_t j = 0; j != 2; ++j)
        if (al[i, j] != sc[i, j])
          return false;

    C sep2[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
    C out2[] = {{}, {}, {}, {}};
    std::mdspan s2(sep2, 2, 2);
    std::mdspan o2(out2, 2, 2);
    std::linalg::hermitian_matrix_product(hov, hmv, std::linalg::lower_triangle, s2, o2);

    C ali2[] = {{10.0, 0.0}, {20.0, 0.0}, {30.0, 0.0}, {40.0, 0.0}};
    std::mdspan a2(ali2, 2, 2);
    std::linalg::hermitian_matrix_product(hov, hmv, std::linalg::lower_triangle, a2, a2);
    for (size_t i = 0; i != 2; ++i)
      for (size_t j = 0; j != 2; ++j)
        if (a2[i, j] != o2[i, j])
          return false;
  }

  return true;
}

constexpr bool test() {
  return test_matrix_product() && test_symmetric_matrix_product() && test_hermitian_matrix_product() &&
         test_triangular_matrix_product() && test_matrix_vector_product() && test_lower_and_implicit_branches();
}

int main(int, char**) { return test() ? 0 : 1; }
