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

// [linalg.syn]: the algorithms have the template-parameter lists of the synopsis (so explicit template arguments
// work) and execution-policy overloads that take part in overload resolution only for valid arguments;
// [linalg.algs.blas1.dot], [linalg.algs.blas1.nrm2], [linalg.algs.blas1.matfrobnorm]: intermediate terms use the
// precision of Scalar when it is higher than that of the value type.

#include <cassert>
#include <complex>
#include <concepts>
#include <execution>
#include <linalg>
#include <mdspan>
#include <type_traits>

template <class V>
concept policy_dot = requires(V v) { std::linalg::dot(std::execution::seq, v, v); };
template <class V>
concept sequential_dot = requires(V v) { std::linalg::dot(v, v); };

using V1 = std::mdspan<double, std::extents<int, 2>>;
using V3 = std::mdspan<double, std::extents<int, 2, 2, 2>>;
using M2 = std::mdspan<double, std::extents<int, 2, 2>>;

static_assert(policy_dot<V1>);
static_assert(!policy_dot<V3>); // no hard error while testing the constraints
static_assert(!sequential_dot<V3>);

int main(int, char**) {
  // explicit template arguments
  double a[2] = {1, 2}, b[4] = {1, 2, 3, 4}, c[2] = {0, 0};
  V1 v(a), out(c);
  M2 m(b);
  assert((std::linalg::dot<V1, V1, double>(v, v, 0.) == 5.0));
  assert((std::linalg::dotc<V1, V1, double>(v, v, 0.) == 5.0));
  std::linalg::matrix_vector_product<M2, V1, V1>(m, v, out);
  assert(out[0] == 5.0 && out[1] == 11.0);
  std::linalg::scale<double, V1>(2., v);
  assert(a[0] == 2.0 && a[1] == 4.0);
  assert((std::linalg::dot<std::execution::sequenced_policy const&, V1, V1, double>(std::execution::seq, v, v, 0.) ==
          20.0));
  assert((std::linalg::vector_two_norm<V1, double>(v, 0.) == std::sqrt(20.0)));

  // the sequential and policy overloads with the same argument lists
  static_assert(std::same_as<decltype(std::linalg::dot(v, v)), double>);
  static_assert(std::same_as<decltype(std::linalg::dot(v, v, 0.f)), float>);
  static_assert(std::same_as<decltype(std::linalg::vector_two_norm(v)), double>);

  // precision: float elements, double Scalar
  float f[] = {0x1.000002p0f};
  std::mdspan<float, std::extents<int, 1>> fv(f);
  const double widened = double(f[0]) * double(f[0]);
  assert(std::linalg::dot(fv, fv, 0.0) == widened);
  assert(std::linalg::dotc(fv, fv, 0.0) == widened);
  assert(std::linalg::vector_two_norm(fv, 0.0) == double(f[0]));
  std::mdspan<float, std::extents<int, 1, 1>> fm(f);
  assert(std::linalg::matrix_frob_norm(fm, 0.0) == double(f[0]));
  // complex elements, complex<double> Scalar
  std::complex<float> cf[] = {{0x1.000002p0f, 0.f}};
  std::mdspan<std::complex<float>, std::extents<int, 1>> cv(cf);
  const std::complex<double> cw = std::complex<double>(cf[0]) * std::complex<double>(cf[0]);
  assert(std::linalg::dot(cv, cv, std::complex<double>{}) == cw);
  assert(std::linalg::vector_two_norm(cv, 0.0) == double(f[0]));
  // no widening needed
  assert(std::linalg::dot(fv, fv, 0.0f) == f[0] * f[0]);
  return 0;
}
