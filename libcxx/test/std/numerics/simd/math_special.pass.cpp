//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

#include <cassert>
#include <cmath>
#include <cmath>
#include <concepts>
#include <simd>
#include <type_traits>

// Equal lanes, or NaN in both: a lane outside the domain of the scalar function is a NaN (a domain error).
template <class T>
bool same_lane(T a, T b) {
  return a == b || (std::isnan(a) && std::isnan(b));
}

// Compare each lane to the scalar function. These SIMD operations intentionally call that
// function directly, so exact equality also checks the element-wise mapping.
template <class T, std::size_t N>
void check_special_functions() {
  using V = std::simd::vec<T, N>;
  using U = std::simd::rebind_t<unsigned, V>;
  static_assert(std::same_as<decltype(std::simd::riemann_zeta(std::declval<V>())), V>);
  static_assert(std::same_as<decltype(std::riemann_zeta(std::declval<V>())), V>);
  static_assert(std::same_as<decltype(std::simd::hermite(std::declval<U>(), std::declval<V>())), V>);
  static_assert(std::same_as<decltype(std::simd::assoc_laguerre(
                                   std::declval<U>(), std::declval<U>(), std::declval<V>())), V>);
  static_assert(std::same_as<decltype(std::simd::sph_legendre(
                                   std::declval<U>(), std::declval<U>(), std::declval<V>())), V>);
  static_assert(std::same_as<decltype(std::simd::hermite(std::declval<U>(), std::declval<V>())), V>);

  V x([](auto i) { return T(1.25) + T(i) * T(0.125); });
  V y([](auto i) { return T(2.25) + T(i) * T(0.125); });
  V z([](auto i) { return T(0.25) + T(i) * T(0.03125); });
  U n([](auto i) { return static_cast<unsigned>(i % 3 + 1); });
  U m([](auto i) { return static_cast<unsigned>(i % 2); });

#define CHECK_UNARY(_Name, _X)                                                                                       \
  do {                                                                                                               \
    V result = std::simd::_Name(_X);                                                                                 \
    for (std::size_t i = 0; i < N; ++i)                                                                             \
      assert(same_lane(result[i], std::_Name(_X[i])));                                                                        \
  } while (false)
#define CHECK_BINARY(_Name, _X, _Y)                                                                                  \
  do {                                                                                                               \
    V result = std::simd::_Name(_X, _Y);                                                                             \
    for (std::size_t i = 0; i < N; ++i)                                                                             \
      assert(same_lane(result[i], std::_Name(_X[i], _Y[i])));                                                                 \
  } while (false)
#define CHECK_TERNARY(_Name, _X, _Y, _Z)                                                                             \
  do {                                                                                                               \
    V result = std::simd::_Name(_X, _Y, _Z);                                                                         \
    for (std::size_t i = 0; i < N; ++i)                                                                             \
      assert(same_lane(result[i], std::_Name(_X[i], _Y[i], _Z[i])));                                                          \
  } while (false)
#define CHECK_INDEX_UNARY(_Name, _N, _X)                                                                             \
  do {                                                                                                               \
    V result = std::simd::_Name(_N, _X);                                                                             \
    for (std::size_t i = 0; i < N; ++i)                                                                             \
      assert(same_lane(result[i], std::_Name(_N[i], _X[i])));                                                                 \
  } while (false)
#define CHECK_INDEX_BINARY(_Name, _N, _M, _X)                                                                        \
  do {                                                                                                               \
    V result = std::simd::_Name(_N, _M, _X);                                                                         \
    for (std::size_t i = 0; i < N; ++i)                                                                             \
      assert(same_lane(result[i], std::_Name(_N[i], _M[i], _X[i])));                                                          \
  } while (false)

  CHECK_INDEX_BINARY(assoc_laguerre, n, m, x);
  CHECK_INDEX_BINARY(assoc_legendre, n, m, z);
  CHECK_BINARY(beta, x, y);
  CHECK_UNARY(comp_ellint_1, z);
  CHECK_UNARY(comp_ellint_2, z);
  CHECK_BINARY(comp_ellint_3, z, x);
  CHECK_BINARY(cyl_bessel_i, z, x);
  CHECK_BINARY(cyl_bessel_j, z, x);
  CHECK_BINARY(cyl_bessel_k, z, x);
  CHECK_BINARY(cyl_neumann, z, x);
  CHECK_BINARY(ellint_1, z, x);
  CHECK_BINARY(ellint_2, z, x);
  CHECK_TERNARY(ellint_3, z, y, x);
  CHECK_UNARY(expint, x);
  CHECK_INDEX_UNARY(hermite, n, x);
  CHECK_INDEX_UNARY(laguerre, n, x);
  CHECK_INDEX_UNARY(legendre, n, z);
  CHECK_UNARY(riemann_zeta, y);
  CHECK_INDEX_UNARY(sph_bessel, n, x);
  CHECK_INDEX_BINARY(sph_legendre, n, m, x);
  CHECK_INDEX_UNARY(sph_neumann, n, x);

  // Check the qualified std:: re-export without ADL.
  V zeta = std::riemann_zeta(y);
  for (std::size_t i = 0; i < N; ++i)
    assert(zeta[i] == std::riemann_zeta(y[i]));
  V hermite = std::hermite(n, x);
  for (std::size_t i = 0; i < N; ++i)
    assert(hermite[i] == std::hermite(n[i], x[i]));

#undef CHECK_UNARY
#undef CHECK_BINARY
#undef CHECK_TERNARY
#undef CHECK_INDEX_UNARY
#undef CHECK_INDEX_BINARY
}

int main(int, char**) {
  check_special_functions<float, 1>();
  check_special_functions<float, 4>();
  check_special_functions<float, 8>();
  check_special_functions<double, 1>();
  check_special_functions<double, 2>();
  check_special_functions<double, 4>();
  return 0;
}
